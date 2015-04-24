var fs = require('fs'),
	page = require('webpage').create();

page.open('file:///E:/Users/Nathan.Typhon/Desktop/sifteo-sdk/examples/cvcwords/letters/phoneme_gen.html',
	function() {
		page.evaluate(function(){});
		fs.write("chunks.html", page.content, 'w');
		phantom.exit();
	}
);
