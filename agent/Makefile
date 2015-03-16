all: agent.c
	gcc -Wall agent.c pcap_conf.c dev_addr.c http_packet_parser.c http_post.c json_event.c json_batch.c -lpcap -lcurl -o agent
clean:
	rm agent
