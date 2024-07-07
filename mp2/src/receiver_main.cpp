/*
 * File:   receiver_main.c
 * Author:
 *
 * Created on
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <queue>

using namespace std;

#define CONTENTSIZE 5000
#define MAXQUEUE 1000

enum state_t
{
    SLOW_START = 5,
    CONJ_AVOIDANCE,
    FAST_RECOV
};

enum packet_t
{
    DATA,
    ACK,
    FIN,
    FINACK
};

typedef struct
{
    int ack_idx;
    int pkt_idx;
    int data_size;
    packet_t pkt_type;
    char data[CONTENTSIZE];
} packet;

struct compare
{
    bool operator()(packet a, packet b)
    {
        return a.pkt_idx > b.pkt_idx;
    }
};

struct sockaddr_in si_me, si_other;
int s;
unsigned int slen;

priority_queue<packet, vector<packet>, compare> pqueue;

void diep(string s)
{
    perror(s.c_str());
    exit(1);
}

void send_ACK(int ack_idx, packet_t pkt_type)
{
    packet pkt;
    pkt.ack_idx = ack_idx;
    pkt.pkt_type = pkt_type;

    if (sendto(s, &pkt, sizeof(packet), 0, (sockaddr *)&si_other, (socklen_t)slen) == -1)
    {
        diep("ACK sendto");
    }
}

void reliablyReceive(unsigned short int myUDPport, char *destinationFile)
{
    int ack_idx = 0;
    slen = sizeof(si_other);

    // create socket for receiving data
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        diep("socket");

    memset((char *)&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me)) == -1)
        diep("bind");

    /* Now receive data and send acknowledgements */
    FILE *fp = fopen(destinationFile, "wb");
    if (fp == NULL)
    {
        diep("fopen");
    }

    while (1)
    {
        packet pkt;
        if (recvfrom(s, &pkt, sizeof(packet), 0, (sockaddr*)&si_other, (socklen_t*)&slen) == -1)
        {
            diep("recvfrom");
        }

        // cout << "got packets" << endl;
        if (pkt.pkt_type == FIN)
        {
            cout << "FIN" << endl;
            send_ACK(ack_idx, FINACK);
            break;
        }
        else if (pkt.pkt_type == DATA)
        {
            if (pkt.pkt_idx > ack_idx)
            {
                cout << "out of order" << endl;
                if (pqueue.size() < MAXQUEUE)
                    pqueue.push(pkt);
            }
            else if (pkt.pkt_idx < ack_idx)
            {
                cout << "dupACK" << endl;
            }
            else
            {
                cout << "received" << endl;
                fwrite(pkt.data, sizeof(char), pkt.data_size, fp);
                ack_idx += pkt.data_size;

                while (!pqueue.empty() && pqueue.top().pkt_idx == ack_idx)
                {
                    packet topPkt = pqueue.top();
                    fwrite(topPkt.data, sizeof(char), topPkt.data_size, fp);
                    ack_idx += topPkt.data_size;
                    pqueue.pop();
                }
            }
            cout << "send ACK" << endl;
            send_ACK(ack_idx, ACK);
        }
        else{
            cout << "got package type" << pkt.pkt_type << endl;
        }
    }
    fclose(fp);
    close(s);

    return;
}

/*
 *
 */
int main(int argc, char **argv)
{

    unsigned short int udpPort;

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int)atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);

    return 0;
}
