# RN Vorbereitung A2

## Aufgabenteil 1

`/sbin/route -`

### Die Paketvermittlung geschieht über den Knotenrechner

1. Die Route auf *Arbeitsrechner* ins Netz *192.168.18.0/24*. 
	* `sudo /sbin/route add -net 192.168.18.0/24 gw 192.168.17.2 eth1`
	* `/sbin/route -`
2. Die Route auf *Zugewieseneer Rechner* ins Netz *192.168.17.0/24*
	* `sudo /sbin/route add -net 192.168.17.0/24 gw 192.168.18.2 eth1`
	* `/sbin/route -`
3. ARP Request und Replay `/sbin/arp`
4. Traceroute `/usr/sbin/traceroute ZielRechnerIP`
5. Record Route Option `ping -R ZielRechnerIP`
6. Datendurchsatz `/usr/local/netperf/netperf -H ZielRechnerIP`

### Die Paketvermittlung geschieht über die ISDN-Strecke

1. Die Route auf *Arbeitsrechner* ins Netz *192.168.18.0/24*. 
	* `sudo /sbin/route add -net 192.168.18.0/24 gw 192.168.17.1 eth1`
	* `/sbin/route -`
2. Die Route auf *Zugewiesener Rechner* ins Netz *192.168.17.0/24*
	* `sudo /sbin/route add -net 192.168.17.0/24 gw 192.168.18.1 eth1`
	* `/sbin/route -`
3. ARP Request und Replay `/sbin/arp`
4. Traceroute `/usr/sbin/traceroute ZielRechnerIP`
5. Record Route Option `ping -R ZielRechnerIP`
6. Datendurchsatz `/usr/local/netperf/netperf -H ZielRechnerIP`

## Aufgabenteil 2

1. Minimale Subnetzmaske per *ifconfig*
	* `sudo /sbin/ifconfig eth1 add ArbeitsRechnerIP netmask 255.255.255.255`
2. Per Ping herausfinden das wir andere Teilnehmer nicht mehr erreichen
	* `ping AndereRechnerIP`
3. Wieder zurück
	* `sudo /sbin/ifconfig eth1 ArbeitsRechnerIP netmask 255.255.255.255`
	* `sudo /sbin/route add 192.168.17.2`
	* `sudo /sbin/route add -net 192.168.18.0/24 gw 192.168.17.2 eth1`
	* `/sbin/route -`







