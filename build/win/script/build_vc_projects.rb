require 'securerandom'
require 'fileutils'

Dir["../../../**/*.cpp"].each do |cppfn|
	name = File.basename(cppfn, ".cpp")
	include_fns = Dir["../../../#{name}/*.h"]
	#puts "\n\n#{name}\n\n"
	#p cppfn
	#p include_fns
	
	# skip existing projects
	path = "../#{name}"
	next if Dir.exists?(path) 
	
	guid = SecureRandom.uuid.upcase
	
	solution_str = <<EOS
	Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "$$NAME$$", "..\\$$NAME$$\\$$NAME$$.vcxproj", "{$$GUID$$}"
	EndProject"
EOS
	solution_str.gsub!("$$NAME$$", name)
	solution_str.gsub!("$$GUID$$", guid)
	puts solution_str
	
	template = IO.read("template.vcxproj")
	
	template.gsub!("$$NAME$$", name)
	template.gsub!("$$GUID$$", guid)
	template.gsub!("$$INCLUDE_FILES$$") { 
		include_fns.map { |fn| "<ClInclude Include=\"#{fn.gsub("/","\\")}\" />" }.join("\n")
	}
	template.gsub!("$$SOURCE_FILES$$") {
		"<ClCompile Include=\"#{cppfn}\" />"
	}
	#puts template
	
	Dir.mkdir(path)
	File.open(path + "/#{name}.vcxproj", "w+") { |f| f.puts(template) }
	FileUtils.copy("template.vcxproj.filters", path + "/#{name}.vcxproj.filters")
end

#~ $$INCLUDE_FILES$$
#~ <ClInclude Include="..\..\..\alignment\alignment.h" />

#~ $$SOURCE_FILES$$
#~ <ClCompile Include="..\..\..\alignment\alignment.cpp" />

#~ $$GUID$$
#~ SecureRandom.uuid

#~ $$NAME$$
