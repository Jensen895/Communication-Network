/*
 * File:   sender_main.c
 * Author:
 *
 * Created on
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <iostream>
#include <queue>
#include <errno.h>

using namespace std;

#define CONTENTSIZE 5000
#define TIMEOUT 100000
#define WEIGHT 256

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

struct sockaddr_in si_me, si_other;
int s;
unsigned int slen;

float sst;
float cw;
state_t state;
int dupACK;
int bytes_to_sent;
int total_pkts;
int idx;
int num_sent;
int num_rec;
FILE *fp;

queue<packet> resend_queue;
queue<packet> send_queue;

void diep(string s)
{
    perror(s.c_str());
    exit(1);
}

void send_pkt(packet *pkt)
{
    cout << "send" << pkt->pkt_idx << endl;
    if (sendto(s, pkt, sizeof(packet), 0, (struct sockaddr *)&si_other, slen) == -1)
    {
        diep("sento()");
    }
}

void dup_ACK_handle()
{
    sst = cw / 2;
    sst = max((float)CONTENTSIZE, sst);
    cw = sst;
    cw += 3 * CONTENTSIZE;
    cw = max((float)CONTENTSIZE, cw);
    // retransmit
    state = FAST_RECOV;
    if (!resend_queue.empty())
    {
        send_pkt(&resend_queue.front());
    }
    dupACK = 0;
}

void stateChange()
{
    switch (state)
    {
    case SLOW_START:
        // dup ACK
        if (dupACK >= 3)
        {
            dup_ACK_handle();
        }
        // new ACK
        else if (dupACK == 0)
        {
            if (cw >= sst)
            {
                state = CONJ_AVOIDANCE;
                return;
            }
            cw += CONTENTSIZE;
            cw = max((float)CONTENTSIZE, cw);
        }
        break;
    case CONJ_AVOIDANCE:
        if (dupACK >= 3)
        {
            dup_ACK_handle();
        }
        else if (dupACK == 0)
        {
            cw += CONTENTSIZE * floor(1.0 * CONTENTSIZE / cw);
            cw = max((float)CONTENTSIZE, cw);
        }
        break;
    case FAST_RECOV:
        if (dupACK > 0)
        {
            cw += CONTENTSIZE;
            return;
        }
        else if (dupACK == 0)
        {
            cw = sst;
            cw = max((float)CONTENTSIZE, cw);
            state = CONJ_AVOIDANCE;
        }
        break;
    default:
        break;
    }
}

void send_func()
{
    if (bytes_to_sent == 0)
    {
        return;
    }
    char buf[CONTENTSIZE];
    memset(buf, 0, CONTENTSIZE);
    packet pkt;

    for (int i = 0; i < ceil((cw - resend_queue.size() * CONTENTSIZE) / CONTENTSIZE); i++)
    {
        // cout << "add to queue" << endl;
        int send_s = min(bytes_to_sent, CONTENTSIZE);
        // cout << "readfile" << endl;
        int bytesRead = fread(buf, sizeof(char), send_s, fp);
        if(bytesRead == -1){
            diep("fread");
        }

        if (bytesRead > 0)
        {
            pkt.pkt_type = DATA;
            pkt.data_size = bytesRead;
            pkt.pkt_idx = idx;
            memcpy(pkt.data, &buf, bytesRead);
            // cout << "send_queue" << endl;
            send_queue.push(pkt);
            // cout << "resend_queue" << endl;
            resend_queue.push(pkt);
            idx += bytesRead;
            bytes_to_sent -= bytesRead;
        }
        else{
            cout << "no file" << endl;
        }
    }
    while (!send_queue.empty())
    {
        // cout << "first trial" << endl;
        send_pkt(&send_queue.front());
        send_queue.pop();
        num_sent++;
    }
}

void ack_handle(packet *pkt)
{
    // old ACK
    if (pkt->ack_idx < resend_queue.front().pkt_idx)
    {
        return;
    }
    // dupACK
    else if (pkt->ack_idx == resend_queue.front().pkt_idx)
    {
        dupACK++;
        stateChange();
    }
    else
    {
        dupACK = 0;
        stateChange();
        int num_pkt = ceil((pkt->ack_idx - resend_queue.front().pkt_idx) / (1.0 * CONTENTSIZE));
        int c = 0;
        num_rec += num_pkt;
        while (!resend_queue.empty() && c < num_pkt)
        {
            resend_queue.pop();
            c++;
        }
        cout << "finish ack" << endl;
        send_func();
    }
}

void timeout_handle()
{
    cout << "timeout" << endl;
    sst = cw / 2;
    sst = max((float)CONTENTSIZE * WEIGHT, sst);
    cw = CONTENTSIZE * WEIGHT;
    state = SLOW_START;

    queue<packet> t = resend_queue;
    for (int i = 0; i < 32; i++)
    {
        if (!t.empty())
        {
            send_pkt(&t.front());
            t.pop();
        }
    }
    dupACK = 0;
}

void end_connect()
{
    cout << "end" << endl;
    packet pkt;
    char buf[sizeof(packet)];
    pkt.pkt_type = FIN;
    pkt.data_size = 0;
    memset(pkt.data, 0, CONTENTSIZE);
    send_pkt(&pkt);

    while (1)
    {
        slen = sizeof(si_other);
        if (recvfrom(s, buf, sizeof(packet), 0, (struct sockaddr *)&si_other, &slen) == -1)
        {
            if (errno != EAGAIN || errno != EWOULDBLOCK)
            {
                diep("recvfrom()");
            }
            else
            {
                pkt.pkt_type = FIN;
                pkt.data_size = 0;
                memset(pkt.data, 0, CONTENTSIZE);
                send_pkt(&pkt);
            }
        }
        else
        {
            packet ackPkt;
            memcpy(&ackPkt, buf, sizeof(packet));
            if (ackPkt.pkt_type == FINACK)
            {
                pkt.pkt_type = FINACK;
                pkt.data_size = 0;
                send_pkt(&pkt);
                break;
            }
        }
    }
}

void reliablyTransfer(char *hostname, unsigned short int hostUDPport, char *filename, unsigned long long int bytesToTransfer)
{
    // Open the file
    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        diep("fopen");
    }

    /* Determine how many bytes to transfer */
    slen = sizeof(si_other);
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    // Clear the server_addr structure and set its values
    memset((char*) &si_other, 0, slen);
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0)
    {
        perror("inet_pton");
        exit(1);
    }

    total_pkts = ceil(1.0 * bytesToTransfer / CONTENTSIZE);
    dupACK = 0;
    idx = 0;
    num_sent = 0;
    num_rec = 0;
    state = SLOW_START;
    bytes_to_sent = bytesToTransfer;
    cw = CONTENTSIZE;
    sst = cw * 512.0;

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = TIMEOUT;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
    {
        diep("setsockopt");
    }

    /* Send data and receive acknowledgements on s*/
    packet pkt;
    // cout <<"send:" << endl;
    send_func();

    // if dropped pkts or dropped ACKs
    while (num_sent < total_pkts || num_rec < total_pkts)
    {
        if ((recvfrom(s, &pkt, sizeof(packet), 0, NULL, NULL)) == -1)
        {
            if (errno != EAGAIN || errno != EWOULDBLOCK)
            {
                diep("recvfrom()");
            }
            // dropped pkt
            if (!resend_queue.empty())
            {
                timeout_handle();
            }
        }
        else
        {
            if (pkt.pkt_type == ACK)
            {
                ack_handle(&pkt);
            }
        }
    }
    end_connect();
    fclose(fp);

    return;
}

/*
 *
 */
int main(int argc, char **argv)
{

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5)
    {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int)atoi(argv[2]);
    numBytes = atoll(argv[4]);

    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);

    return (EXIT_SUCCESS);
}
