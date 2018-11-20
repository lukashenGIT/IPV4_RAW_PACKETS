#include "packet_maker.h"

#define MAX_CMP_WORDS 15
#define DG_BUFFER 4096




unsigned short csum(char *buf, int nwords)
//Calculate checksum of header.
{
        unsigned long sum;
        for(sum=0; nwords>0; nwords--)
                sum += *buf++;
        sum = (sum >> 16) + (sum &0xffff);
        sum += (sum >> 16);
        return (unsigned short)(~sum);
}



int main(int argc, char * argv[]){


	// Make Buffer for datagram and cast to the IP_header struct (defined in packet_maker.h).
	char datagram[DG_BUFFER];
	char payload_buffer[DG_BUFFER-20]; // Assuming header is always 20 bytes.
	struct IP_header * iph = (struct IP_header *) datagram;

	//Zero the buffer.
	memset(datagram, 0, 64);

	//Set number of words in header. 5 for standard 20 byte header.
	int nwords = 5;



	//Set values of IP header. See rfc791 for meaning of all values.
	//Also sets default fallback values if flags are not specified for a specific header field.
	iph->tos = 0;					// 0 is normal service. 
	iph->ihl = nwords;				// 5 is default header without options.
	iph->version = 4;				// 4 is ipv4, Ipv6 not implemented.
	iph->total_len = 20;			// 20 bytes if no payload
	iph->identification = 65535;	// Max value for recognization
	iph->flags = 3;					// 3 bits 0b010 (Do not fragment, and no more fragments.)
	iph->offset = 0;				// No offset unless fragmentation.
	iph->ttl = 128;					// 8 bits 0x80 = 128
	iph->protocol = 4;				// 4 for ipv4 
	iph->src_address = inet_addr("127.0.0.1");	// 32 bits spoofed. 123.123.123.123
	iph->dst_address = inet_addr("255.255.255.255");	// 32 bits localhost: 127.0.0.1

	//Update header based on options.
	iph = parse_options_and_modify_struct(argc, argv, iph);

	//Calculate and update checksum based on updated header (from options)
	iph->checksum = csum(datagram, nwords);

	// Create socket.
	int sd;
	sd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);
	if(sd < 0){
		perror("Error creating socket.");
		exit(0);
	}

	// Set the socket to include a custom header.
	int on = 1;
	if(setsockopt(sd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0){
		perror("setsockopt");
		close(sd);
		exit(0);
	}

	//setup src socket details.
	struct sockaddr_in sin;
	sin.sin_family = PF_INET;
	sin.sin_addr.s_addr = iph->src_address; // Can be set to your interface ip with src_address being somethin else if you want to forge packets.

	//Read payload from stdin
	read(STDIN_FILENO, payload_buffer, sizeof(payload_buffer));

	//Append to buffer after header.
	memcpy((datagram + 20), payload_buffer, sizeof(payload_buffer));

	

	if(sendto(sd, datagram, sizeof(datagram), 0, (struct sockaddr *)&sin, sizeof(sin)) < 0){
		perror("sendto");
		close(sd);
		exit(0);
	}
	
	close(sd);
	return 0;
}
