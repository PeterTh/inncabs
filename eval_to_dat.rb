#!/usr/bin/env ruby
#
# run in date or date/1-th_per_core   (or 2-th..) where g++-<#samples> resides
# run example:   (where hpx:1th/core is a label and 5 is the #samples)
#
#   example: eval_to_dat.rb hpx:1th/core 5
#

label=ARGV[0] # used for label
samples=ARGV[1] # samples run 

#res_names = %w(clang clang_limit64 gcc gcc_limit64 win64)
res_names = %w(clang gcc win64)
res_names = %w(g++)
itime=0
istdev=1

launch_types = %w(deferred optional async)
launch_types = %w(async)
benchmarks = %w(fft fib floorplan health intersim nqueens pyramids round sort sparselu strassen uts)
benchmarks = %w(intersim)

cores = [1,2,4,6,8,10,12,14,16,18,20]

#$max_result = 900000
$max_result = "?" 
$width = 18

def read_result(results, benchname, launchtype, numcpus,column)
	numcpus = 1 if launchtype == "deferred"
	return $max_result unless results.include?(benchname)
	benchresults = results[benchname]
	return $max_result unless benchresults.include?(launchtype)
	launchresults = benchresults[launchtype]
	return $max_result unless launchresults.include?(numcpus)
	result = launchresults[numcpus][column]
	return $max_result if result == 0 
	return result
end

benchmarks.each do |benchname|
	# Benchmark name
	puts "\n\n#" + benchname
	# Headers
	print "Cores , ".rjust(8)
	res_names.each do |resname|
		launch_types.each do |launchtype|
			#print "#{resname}:#{launchtype} ,".rjust($width) #original label
			print "#{benchname}:#{label} ,".rjust($width)
			print "#{benchname}:#{label}:stdev,".rjust($width)
		end
	end
	puts
	# Content
    cores.each do |i|
		print "#{i} , ".rjust(8)
		res_names.each do |resname|
            file ="#{resname}-#{samples}/results.rb"
			results = eval(IO.read(file))
            launch_types.each do |launchtype|
				print "#{read_result(results, benchname, launchtype, i,$itime)} ,".rjust($width)
				print "#{read_result(results, benchname, launchtype, i,$istdev)} ,".rjust($width)
			end
		end
		puts
	end
end
	
