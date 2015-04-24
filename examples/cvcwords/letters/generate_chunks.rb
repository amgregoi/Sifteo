#require 'phantomjs'
require 'rubygems'
require 'nokogiri'
require 'open-uri'
require 'data-uri'
require 'uri'

URL = "FILE:///#{Dir.pwd}/phenome_gen.html"

chunks = [];

# Generate individual letters
aCode = "a"[0].ord
(0..25).each do |i|
	chunks << (aCode + i).chr
end

# Word family chunks
chunks << "ab"
chunks << "ad"
chunks << "ag"
chunks << "am"
chunks << "an"
chunks << "ap"
chunks << "at"
chunks << "ed"
chunks << "ell"
chunks << "en"
chunks << "et"
chunks << "ig"
chunks << "id"
chunks << "in"
chunks << "ill"
chunks << "ip"
chunks << "it"
chunks << "op"
chunks << "og"
chunks << "op"
chunks << "ot"
chunks << "ub"
chunks << "ug"
chunks << "um"
chunks << "un"

# multiple letter graphemes
chunks << "bb"
chunks << "ck"
chunks << "ch"
chunks << "dd"
chunks << "ff"
chunks << "ph"
chunks << "gh"
chunks << "dge"
chunks << "ge"
chunks << "ll"
chunks << "mm"
chunks << "mb"
chunks << "kn"
chunks << "pp"
chunks << "rr"
chunks << "wr"
chunks << "ss"
chunks << "ce"
chunks << "se"
chunks << "sc"
chunks << "tt"
chunks << "ve"
chunks << "wh"
chunks << "zz"
chunks << "ze"
chunks << "sh"
chunks << "si"
chunks << "ti"
chunks << "tch"
chunks << "th"
chunks << "ng"



# Write to chunks.js
File.open("chunks.js", "w").write("var chunks = [#{chunks.collect{|c| "\"#{c}\""}.join(",")}];")

# Generate images with phantomjs
system "phantomjs scrape.js"

# Open and save files
Nokogiri::HTML(File.open("chunks.html")).xpath("//img/@src").each_with_index do |src, i|
	File.open("img/#{chunks[i]}.png", 'wb') do |f|	
		uri = src.content();
		# decode the dataURL
		#puts src.split(',') #.split(',')[1]
		f.write DataURI::decode(uri)
	end
end

# Create Lua asset script
File.open("img/assets.lua", "w") do |f|
	f.write "Alphabet = image{ {#{chunks.collect{|c| "\"assets/#{c}.png\""}.join(", ")} } }"
end