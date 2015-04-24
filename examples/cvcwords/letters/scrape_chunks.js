var page = require('webpage').create();

page.open('file:///E:/Users/Nathan.Typhon/Desktop/sifteo-sdk/examples/cvcwords/letters/phoneme_gen.html',
	function() {
		
		page.includeJs('//ajax.googleapis.com/ajax/libs/jquery/1.10.2/jquery.min.js', function() {

        var images = page.evaluate(function() {
            var images = [];

            $('img').each(function() {
                var img = getImgDimensions($(this));
                images.push(img);
            });

            return images;
        });

        images.forEach(function(imageObj, index, array){
            page.clipRect = imageObj;
            page.render('images/'+index+'.png')
        });

        phantom.exit();
    });
});

function getImgDimensions($i) {
    return {
        top : $i.offset().top,
        left : $i.offset().left,
        width : $i.width(),
        height : $i.height()
    }
}