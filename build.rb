#!/usr/bin/env ruby

repo_dir = './'

require_relative repo_dir + 'color.rb'

compiler = "g++"
CPPFLAGS = "-std=c++11 -Wall -pthread"

#command line arguments
clang=ARGV.include?("--clang")
gpp=ARGV.include?("--g++")
debug = ARGV.include?("--dbg")
parallel = ARGV.include?("--par")
clean = ARGV.include?("--clean")
ARGV.delete_if { |e| e.start_with?("--") }


threads = []
if (clang) 
    compiler = "clang++"
end
print "Compiling with ", compiler, "\n"

Dir[repo_dir + "/**/*.cpp"].each do |cppfile|
	next if !ARGV.empty? && !ARGV.any? { |arg| cppfile =~ /#{arg}/ }
	job = lambda do
		fname = File.basename(cppfile, ".cpp")
		outfname =  "./bin/" + fname
		if(clean) then
			File.delete(outfname) if File.exists?(outfname)
			puts "======== Cleaned " + fname.red.bold
			return
		end
		command = "#{compiler} #{cppfile} -o #{outfname} #{CPPFLAGS}"
		#command = "/software-local/insieme-libs/gcc-4.9.0/bin/g++ #{cppfile} -o #{outfname} -std=c++11 -Wall -lpthread"
		command += " -g3" if debug
		command += " -O3" if !debug
		puts "======== Building " + fname.green.bold + (debug && " debug" || " release") + " version"
		`#{command}`
	end
	if(parallel) then threads << Thread.new { job.call }
	else job.call end
end

threads.each { |t| t.join }

