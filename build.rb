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

#specify the apps to compile
apps = ["alignment", "fft", "fib", "floorplan", "health", "intersim", 
        "nqueens", "pyramids", "qap", "round", "sort", "sparselu", 
        "strassen", "uts"]

apps.each do |app|
	next if !ARGV.empty? && !ARGV.any? { |arg| app =~ /#{arg}/ }
	job = lambda do
		outfname =  "./bin/" + app
        if(clean) then
			File.delete(outfname) if File.exists?(outfname)
			puts "======== Cleaned " + app.red.bold
			return
		end
            command = "#{compiler} #{repo_dir + app + "/" + app + ".cpp"} -o #{outfname} #{CPPFLAGS}"
		if app == "alignment"
            command += " -ltcmalloc"
        end
#command = "/software-local/insieme-libs/gcc-4.9.0/bin/g++ #{app} -o #{outfname} -std=c++11 -Wall -lpthread"
		command += " -g3" if debug
		command += " -O3" if !debug
		puts "======== Building " + app.green.bold + (debug && " debug" || " release") + " version"
        #puts command
		`#{command}`
	end
	if(parallel) then threads << Thread.new { job.call }
	else job.call end
end

threads.each { |t| t.join }

