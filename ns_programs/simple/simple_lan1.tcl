#Create a simulator object
set ns [new Simulator]

#Define different colors for the data flows (for NAM
$ns color 1 Blue
$ns color 2 Red

#Open the trace file
set tf [open simple_lan1_trace.tr w]
$ns trace-all $tf

#Open the NAM trace file
set nf [open simple_lan1_nam.nam w]
$ns namtrace-all $nf

#Define a 'finish' procedure
proc finish {} {
	global ns tf nf
	$ns flush-trace
	#Close NAM trace file
	close $tf
	close $nf
	#Execute NAM on the trace file
	exec nam simple_lan1_nam.nam &
	exit 0
}

#Create four nodes
set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

#Create links between the nodes
$ns make-lan "$n0 $n1 $n2 $n3" 10Mb 20ms LL Queue/DropTail Mac/802_3

#Give node position (for NAM)
#$ns node $n0 orient left-up
#$ns node $n1 orient right-up
#$ns node $n2 orient left-down
#$ns node $n3 orient right-down

#Monitor the queue for link (n2-n3) for (NAM)

#Setup a TCP connection
set tcp [new Agent/TCP]
$tcp set class_ 2
$ns attach-agent $n0 $tcp
set sink [new Agent/TCPSink]
$ns attach-agent $n3 $sink
$ns connect $tcp $sink
$tcp set fid_ 1

#Setup a FTP over TCP connection
set ftp [new Application/FTP]
$ftp attach-agent $tcp
$ftp set type_ FTP

#Setup a UDP connection
set udp [new Agent/UDP]
$ns attach-agent $n1 $udp
set null [new Agent/Null]
$ns attach-agent $n3 $null
$ns connect $udp $null
$udp set fid_ 2

#Setup a CBR over UDP connection
set cbr [new Application/Traffic/CBR]
$cbr attach-agent $udp
$cbr set type_ CBR
$cbr set packet_Size_ 64
$cbr set rate_ 1mb
$cbr set random_ false

#Schedule events for the CBR and FTP agents
$ns at 0.1 "$cbr start"
$ns at 1.0 "$ftp start"
$ns at 4.0 "$cbr start"
$ns at 4.5 "$cbr start"

#Detach tcp and sink agents (not really necessary)
$ns at 4.5 "$ns detach-agent $n0 $tcp; $ns detach-agent $n3 $sink"

#Call the finish procedure after 5 seconds of simulation time
$ns at 5.0 "finish"

#Print CBR packet size and interval
puts "CBR Packet size = [$cbr set packet_size_]"
puts "CBR Interval = [$cbr set interval_]"

#Run the simulation
$ns run
