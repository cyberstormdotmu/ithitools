/*
* Author: Christian Huitema
* Copyright (c) 2017, Private Octopus, Inc.
* All rights reserved.
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Private Octopus, Inc. BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef DNSSTAT_H
#define DNSSTAT_H

#include <stdint.h>
#include <stdio.h>
#include "DnsStatHash.h"
#include "AddressFilter.h"
#include "HashBinGeneric.h"

/*
 * List of registry definitions 
 */

#define	REGISTRY_DNS_CLASSES	1
#define	REGISTRY_DNS_RRType	2
#define	REGISTRY_DNS_OpCodes	3
#define	REGISTRY_DNS_RCODES	4
#define	REGISTRY_DNS_AFSDB_RRSubtype	5
#define	REGISTRY_DNS_DHCID_RRIdType	6
#define	REGISTRY_DNS_LabelType	7
#define	REGISTRY_EDNS_OPT_CODE	8
#define	REGISTRY_DNS_Header_Flags	9
#define	REGISTRY_EDNS_Header_Flags	10
#define	REGISTRY_EDNS_Version_number	11
#define	REGISTRY_DNS_CSYNC_Flags	12
#define	REGISTRY_DNSSEC_Algorithm_Numbers	13
#define	REGISTRY_DNSSEC_KEY_Prime_Lengths	14
#define	REGISTRY_DNS_Q_CLASSES	15
#define	REGISTRY_DNS_Q_RRType	16
#define	REGISTRY_DNSSEC_KEY_Well_Known_Primes	17
#define	REGISTRY_EDNS_Packet_Size 18
#define	REGISTRY_DNS_Query_Size 19
#define	REGISTRY_DNS_Response_Size 20
#define	REGISTRY_DNS_TC_length 21
#define	REGISTRY_TLD_query 22
#define	REGISTRY_TLD_response 23
#define	REGISTRY_DNS_error_flag 24
// #define	REGISTRY_TLD_error_class 25
#define	REGISTRY_DNS_txt_underline 26
#define REGISTRY_DNS_root_QR 27
#define REGISTRY_DNS_LeakByLength 28
#define REGISTRY_DNS_LeakedTLD 29
#define REGISTRY_DNS_RFC6761TLD 30
#define REGISTRY_DNS_UsefulQueries 31

#define DNS_REGISTRY_ERROR_RRTYPE (1<<0)
#define DNS_REGISTRY_ERROR_RRCLASS (1<<1)
#define DNS_REGISTRY_ERROR_OPCODE (1<<2)
#define DNS_REGISTRY_ERROR_RCODE (1<<3)
#define DNS_REGISTRY_ERROR_KALGO (1<<4)
#define DNS_REGISTRY_ERROR_OPTO (1<<5)
#define DNS_REGISTRY_ERROR_TLD (1<<6)
#define DNS_REGISTRY_ERROR_LABEL (1<<7)
#define DNS_REGISTRY_ERROR_FORMAT (1<<8)

#define DNS_OPCODE_QUERY 0
#define DNS_RCODE_NOERROR 0
#define DNS_RCODE_NXDOMAIN 3


enum DnsStatsFlags
{
    dnsStateFlagFilterLargeUsers = 1,
    dnsStateFlagCountTld = 2,
    dnsStateFlagCountQueryParms = 4,
    dnsStateFlagCountUnderlinedNames = 8,
    dnsStateFlagCountPacketSizes = 16
};

class TldAsKey
{
public:
    TldAsKey(uint8_t * tld, size_t tld_len);
    ~TldAsKey();

    bool IsSameKey(TldAsKey* key);
    uint32_t Hash();
    TldAsKey* CreateCopy();
    void Add(TldAsKey* key);

    TldAsKey * HashNext;
    TldAsKey * MoreRecentKey;
    TldAsKey * LessRecentKey;

    size_t tld_len;
    uint8_t tld[65];
    uint32_t count;
    uint32_t hash;

    static void CanonicCopy(uint8_t * tldDest, size_t tldDestMax, size_t * tldDestLength, 
        uint8_t * tldSrce, size_t tldSrceLength);
};

class TldAddressAsKey
{
public:
    TldAddressAsKey(uint8_t * addr, size_t addr_len, uint8_t * tld, size_t tld_len);
    ~TldAddressAsKey();

    bool IsSameKey(TldAddressAsKey* key);
    uint32_t Hash();
    TldAddressAsKey* CreateCopy();
    void Add(TldAddressAsKey* key);

    TldAddressAsKey * HashNext; 

    size_t addr_len;
    uint8_t addr[16];
    size_t tld_len;
    uint8_t tld[65];
    uint32_t count;
    uint32_t hash;
};

class DnsStats
{
public:
    DnsStats();
    ~DnsStats();

    DnsStatHash hashTable;
    AddressFilter rootAddresses;
    LruHash<TldAsKey> tldLeakage;
    BinHash<TldAddressAsKey> queryUsage;

    void SubmitPacket(uint8_t * packet, uint32_t length, int ip_type, uint8_t* ip_header);

    bool ExportToCsv(char const * fileName);

    uint32_t max_tld_leakage_count; 
    uint32_t max_tld_leakage_table_count;
    uint32_t max_query_usage_count;
    uint32_t dnsstat_flags;
    int record_count; 
    int query_count;
    int response_count;
    uint32_t error_flags;


    static bool IsRfc6761Tld(uint8_t * tld, size_t length);
    static void SetToUpperCase(uint8_t * domain, size_t length);

private:
    int SubmitQuery(uint8_t * packet, uint32_t length, uint32_t start, bool is_response);
    int SubmitRecord(uint8_t * packet, uint32_t length, uint32_t start, 
        uint32_t * e_rcode, uint32_t * e_length, bool is_response);
    int SubmitName(uint8_t * packet, uint32_t length, uint32_t start, bool should_tabulate);

    void SubmitOPTRecord(uint32_t flags, uint8_t * content, uint32_t length, uint32_t * e_rcode);
    void SubmitKeyRecord(uint8_t * content, uint32_t length);
    void SubmitRRSIGRecord(uint8_t * content, uint32_t length);
    void SubmitDSRecord(uint8_t * content, uint32_t length);

    void SubmitRegistryNumberAndCount(uint32_t registry_id, uint32_t number, uint32_t count);
    void SubmitRegistryNumber(uint32_t registry_id, uint32_t number);
    void SubmitRegistryStringAndCount(uint32_t registry_id, uint32_t length, uint8_t * value, uint32_t count);
    void SubmitRegistryString(uint32_t registry_id, uint32_t length, uint8_t * value);
#if 0
    void PrintRRType(FILE* F, uint32_t rrtype);
    void PrintRRClass(FILE* F, uint32_t rrclass);
    void PrintOpCode(FILE* F, uint32_t opcode);
    void PrintRCode(FILE* F, uint32_t rcode);
    void PrintDnsFlags(FILE* F, uint32_t flag);
    void PrintEDnsFlags(FILE* F, uint32_t flag);
    void PrintKeyAlgorithm(FILE* F, uint32_t algo);
    void PrintOptOption(FILE* F, uint32_t option);
    void PrintErrorFlags(FILE* F, uint32_t flags);
    void PrintTldErrorClass(FILE* F, uint32_t tld_error_class);

    void CheckRRType(uint32_t rrtype);
    void CheckRRClass(uint32_t rrclass);
    void CheckOpCode(uint32_t opcode);
    void CheckRCode(uint32_t rcode);
    void CheckKeyAlgorithm(uint32_t algo);
    void CheckOptOption(uint32_t option);
    bool CheckTld(uint32_t length, uint8_t * lower_case_tld);
#endif
    int CheckForUnderline(uint8_t * packet, uint32_t length, uint32_t start);

    int GetTLD(uint8_t * packet, uint32_t length, uint32_t start, uint32_t *offset);

    void NormalizeNamePart(uint32_t length, uint8_t * value, uint8_t * normalized, uint32_t * flags);

    void GetSourceAddress(int ip_type, uint8_t * ip_header, uint8_t ** addr, size_t * addr_length);
    void GetDestAddress(int ip_type, uint8_t * ip_header, uint8_t ** addr, size_t * addr_length);

    void ExportLeakedDomains();
};

#endif /* DNSTAT_H */
