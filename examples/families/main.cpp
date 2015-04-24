/*
 * Phonemic.
 */

#include <sifteo.h>
#include <sifteo/string.h>
#include "assets.gen.h"
using namespace Sifteo;

// METADATA

static Metadata M = Metadata()
    .title("Phonemic")
    .package("com.sifteo.sdk.phonemic", "0.1")
    .icon(Icon)
    .cubeRange(1, CUBE_ALLOCATION);


AssetSlot gMainSlot = AssetSlot::allocate()
    .bootstrap(BootstrapAssets);
	
// STRUCTURES
struct CHUNK
{
	const AssetAudio& phoneme;
	const AssetImage& grapheme;
	const char* letters;
};


// long a


// GLOBALS

static VideoBuffer vbuf[CUBE_ALLOCATION]; // one video-buffer per cube
static char cubeLetters[CUBE_ALLOCATION]; // one letter per cube
static int cubeChunks[CUBE_ALLOCATION]; // each cube gets a chunk
static CubeSet newCubes; // new cubes as a result of paint()
static CubeSet lostCubes; // lost cubes as a result of paint()
static CubeSet reconnectedCubes; // reconnected (lost->new) cubes as a result of paint()
static CubeSet dirtyCubes; // dirty cubes as a result of paint()
static CubeSet activeCubes; // cubes showing the active scene

static Random rand; // random number generator
static AssetLoader loader; // global asset loader (each cube will have symmetric assets)
static AssetConfiguration<1> config; // global asset configuration (will just hold the bootstrap group)

// FUNCTIONS

static void playSfx(const AssetAudio& sfx) {
    static int i=0;
    AudioChannel(i).play(sfx);
    i = 1 - i;
}

static void playPhoneme(const AssetAudio& sfx) {
	AudioChannel(3).play(sfx);
}

static Int2 getRestPosition(Side s) {
    // Look up the top-left pixel of the bar for the given side.
    // We use a switch so that the compiler can optimize this
    // however if feels is best.
    switch(s) {
    case TOP: return vec(64 - Bars[0].pixelWidth()/2,0);
    case LEFT: return vec(0, 64 - Bars[1].pixelHeight()/2);
    case BOTTOM: return vec(64 - Bars[2].pixelWidth()/2, 128-Bars[2].pixelHeight());
    case RIGHT: return vec(128-Bars[3].pixelWidth(), 64 - Bars[3].pixelHeight()/2);
    default: return vec(0,0);
    }
}

static void pickLetters() {
	Random random;
	char words[209][4] = {"god","sap","sat","sad","rat","rap","ram","rag","nap","mat","map","mad","lap","lag","lad","fat","fan","fad","fin","fit","lid","lip","lit","mid","nit","nip","rid","rig","rim","rip","sin","sip","log","mom","mop","nod","rod","rot","sod","fun","mud","mum","nut","rug","rut","sum","sun","fed","led","leg","met","net","bag","bad","bam","bat","cap","cab","dad","gas","gag","ham","hat","jab","jam","pan","pat","tab","tag","tan","tap","bid","dig","dip","hid","hit","hip","jig","kin","kid","pin","pit","pig","tin","tip","cop","con","dog","hop","hog","job","jog","pot","pop","top","bug","bud","bum","cup","cub","dud","dug","gun","hum","jug","pup","tub","tug","beg","bed","bet","hen","jet","pen","pet","peg","pep","had","but","not","can","him","did","get","man","set","put","big","men","got","run","let","cut","hot","ran","red","sit","six","ten","yes","bit","bun","cat","dam","den","dim","gut","hug","hut","kit","tam","van","wet","win","bib","box","bus","dot","fib","fix","fog","fox","gum","lug","mob","mug","rub","sub","wag","wax","wig","zap","mix","yet","zip","bob","dab","lot","nab","pal","rob","sag","tot","web","yak","yum","ban","cog","cot","fig","gap","nag","pod","rib","sob","tax","vet","yap","zen","bog","cob","cod","hem","jib","jut","lop","pad","wed","sis"};
	int index = random.randrange(209);
	int i = 0;
	
	
	for(CubeID cid : CubeSet::connected()) {
		vbuf[cid].bg0.image(vec(0,0), chunks[cubeChunks[cid]].grapheme);
		i++;
    }
}

static int barSpriteCount(CubeID cid) {
    // how many bars are showing on this cube?
    ASSERT(activeCubes.test(cid));
    int result = 0;
    for(int i=0; i<4; ++i) {
        if (!vbuf[cid].sprites[i].isHidden()) {
            result++;
        }
    }
    return result;
}

static bool showSideBar(CubeID cid, Side s) {
    // if cid is not showing a bar on side s, show it and check if the
    // smiley should wake up
    ASSERT(activeCubes.test(cid));
    if (vbuf[cid].sprites[s].isHidden()) {
        vbuf[cid].sprites[s].setImage(Bars[s]);
        vbuf[cid].sprites[s].move(getRestPosition(s));
        if (barSpriteCount(cid) == 1) {
            vbuf[cid].bg0.image(vec(0,0), chunks[cubeChunks[cid]].grapheme);
        }
        return true;
    } else {
        return false;
    }
}

static bool hideSideBar(CubeID cid, Side s) {
    // if cid is showing a bar on side s, hide it and check if the
    // smiley should go to sleep
    ASSERT(activeCubes.test(cid));
    if (!vbuf[cid].sprites[s].isHidden()) {
        vbuf[cid].sprites[s].hide();
        if (barSpriteCount(cid) == 0) {
            vbuf[cid].bg0.image(vec(0,0), chunks[cubeChunks[cid]].grapheme);
        }
        return true;
    } else {
        return false;
    }
}

static void activateCube(CubeID cid) {
    // mark cube as active and render its canvas
    activeCubes.mark(cid);
    vbuf[cid].initMode(BG0_SPR_BG1);
    vbuf[cid].bg0.image(vec(0,0), chunks[cubeChunks[cid]].grapheme);
    auto neighbors = vbuf[cid].physicalNeighbors();
    for(int side=0; side<4; ++side) {
        if (neighbors.hasNeighborAt(Side(side))) {
            showSideBar(cid, Side(side));
        } else {
            hideSideBar(cid, Side(side));
        }
    }
}

static void paintWrapper() {
    // clear the palette
    newCubes.clear();
    lostCubes.clear();
    reconnectedCubes.clear();
    dirtyCubes.clear();

    // fire events
    System::paint();

    // dynamically load assets just-in-time
    if (!(newCubes | reconnectedCubes).empty()) {
        AudioTracker::pause();
        playSfx(SfxConnect);
        loader.start(config);
        while(!loader.isComplete()) {
            for(CubeID cid : (newCubes | reconnectedCubes)) {
                vbuf[cid].bg0rom.hBargraph(
                    vec(0, 4), loader.cubeProgress(cid, 128), BG0ROMDrawable::ORANGE, 8
                );
            }
            // fire events while we wait
            System::paint();
        }
        loader.finish();
        AudioTracker::resume();
        AudioTracker::resume();
    }

    // repaint cubes
    for(CubeID cid : dirtyCubes) {
        activateCube(cid);
    }
    
    // also, handle lost cubes, if you so desire :)
}

static void onCubeConnect(void* ctxt, unsigned cid) {
    // this cube is either new or reconnected
    if (lostCubes.test(cid)) {
        // this is a reconnected cube since it was already lost this paint()
        lostCubes.clear(cid);
        reconnectedCubes.mark(cid);
    } else {
        // this is a brand-spanking new cube
        newCubes.mark(cid);
    }
    // begin showing some loading art (have to use BG0ROM since we don't have assets)
    dirtyCubes.mark(cid);
    auto& g = vbuf[cid];
    g.attach(cid);
    g.initMode(BG0_ROM);
    g.bg0rom.fill(vec(0,0), vec(16,16), BG0ROMDrawable::SOLID_BG);
    g.bg0rom.text(vec(1,1), "Hold on!", BG0ROMDrawable::BLUE);
    g.bg0rom.text(vec(1,14), "Adding Cube...", BG0ROMDrawable::BLUE);
}

static void onCubeDisconnect(void* ctxt, unsigned cid) {
    // mark as lost and clear from other cube sets
    lostCubes.mark(cid);
    newCubes.clear(cid);
    reconnectedCubes.clear(cid);
    dirtyCubes.clear(cid);
    activeCubes.clear(cid);
}

static void onCubeRefresh(void* ctxt, unsigned cid) {
    // mark this cube for a future repaint
    dirtyCubes.mark(cid);
}

static bool isActive(NeighborID nid) {
    // Does this nid indicate an active cube?
    return nid.isCube() && activeCubes.test(nid);
}

static void onNeighborAdd(void* ctxt, unsigned cube0, unsigned side0, unsigned cube1, unsigned side1) {
    
	// only join right-left or left-right
	if( (side0 == LEFT && side1 == RIGHT) || (side0 == RIGHT && side1 == LEFT) )
	{
		// update art on active cubes (not loading cubes or base)
		bool sfx = false;
		if (isActive(cube0)) { sfx |= showSideBar(cube0, Side(side0)); }
		if (isActive(cube1)) { sfx |= showSideBar(cube1, Side(side1)); }
		if (sfx) { playSfx(SfxAttach); }
		
		if( spellsWord(cube0) || spellsWord(cube1) ) {
			if(sfx) playSfx( PhonemeShortA );//SfxConnect);
		}
	}
}

static void onNeighborRemove(void* ctxt, unsigned cube0, unsigned side0, unsigned cube1, unsigned side1) {
    // update art on active cubes (not loading cubes or base)
    bool sfx = false;
    if (isActive(cube0)) { sfx |= hideSideBar(cube0, Side(side0)); }
    if (isActive(cube1)) { sfx |= hideSideBar(cube1, Side(side1)); }
    if (sfx) { playSfx(SfxDetach); }
}


static void onCubeTouch(void* ctxt, unsigned cube) {
	playPhoneme(chunks[cubeChunks[cube]].phoneme);
}

void main() {
	int i = 0;
    // initialize asset configuration and loader
    config.append(gMainSlot, BootstrapAssets);
    loader.init();

    // subscribe to events
    Events::cubeConnect.set(onCubeConnect);
    Events::cubeDisconnect.set(onCubeDisconnect);
    Events::cubeRefresh.set(onCubeRefresh);

    Events::neighborAdd.set(onNeighborAdd);
    Events::neighborRemove.set(onNeighborRemove);
	Events::cubeTouch.set(onCubeTouch);
    
    
    // initialize cubes
    AudioTracker::setVolume(0.2f * AudioChannel::MAX_VOLUME);
    AudioTracker::play(Music);
    for(CubeID cid : CubeSet::connected()) {
		cubeLetters[cid] = 'a' + i;
		if(i == 0) cubeChunks[i] = 0;
		if(i == 1) cubeChunks[i] = 1;
		if(i == 2) cubeChunks[i] = 3;
        vbuf[cid].attach(cid);
        activateCube(cid);
		i++;
    }
	
	// pick cube letters
	//pickLetters();
	
    // run loop
    for(;;) {
        paintWrapper();
    }
}
