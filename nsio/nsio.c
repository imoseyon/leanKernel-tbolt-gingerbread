/******
 NetSpector kernel driver, version 2.0
 http://www.hexview.com/android/netspector
 android@hexview.com
******/


#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <net/tcp.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include "nsio.h"

MODULE_AUTHOR("HexView");
MODULE_LICENSE("GPL");
MODULE_VERSION(MY_API_VERSION);
MODULE_DESCRIPTION("NetSpector filter module. http://www.hexview.com/android/netspector");
MODULE_PARM_DESC (fake_vermagic_buffer,"********************************************************************************************************************************************");

static unsigned int max_log 	= MAX_LOG_RECORDS;
static unsigned int max_filters	= MAX_FILTER_RECORDS;
static unsigned int max_data	= MAX_DATA_LEN;
static unsigned int clear_data	= 0;

module_param(max_log, int ,0);
MODULE_PARM_DESC (max_log, "Maximum number of log records");
module_param(max_filters, int ,0);
MODULE_PARM_DESC (max_filters, "Maximum number of filters");
module_param(max_data, int,0);
MODULE_PARM_DESC (max_data, "Maximum size of captured packet data");
module_param(clear_data, int,0);
MODULE_PARM_DESC (clear_data, "Zeroize TCP data upon reset");

static struct nf_hook_ops nfho;

static log_rec_s *conn_list;
static filter_rec_s *filter_list;

static u32 clog_head=0;
static u32 clog_tail=0;
static u32 log_pkt_req;
static u32 conn_count=0;

static u32 api_cmd = API_GET_VERSION;

static u32 filter_count=0;

static u32 master_bypass = 0;

char *strip_leading_spaces(char *str)
{
    while ( *str == ' ' || *str == '\t' ) str++;
    return str;
}

char *strNstr(char *s, char *find, size_t slen)
{
    char c, sc;
    size_t len;

    if ((c = *find++) != '\0') {
        len = strlen(find);
        do {
            do {
                if (slen-- < 1 || (sc = *s++) == '\0')
                    return (NULL);
            } while (sc != c);
            if (len > slen)
                return (NULL);
        } while (strncmp(s, find, len) != 0);
        s--;
    }
    return ((char *)s);
}

char *strstr_nl(char *str, char *ptn)
{
    int i;
    int slen;
    int dlen=strlen(ptn);
    char *eol = strpbrk(str,"\r\n");

    if (! ptn) return str;

    slen =  eol ? (eol-str) : strlen(str);

    for (i=0; i<=(slen-dlen); i++) {
        if (strnicmp(str,ptn,dlen)==0) return str;
        str++;

    }

    return NULL;
}

void clear_log(void)
{
    log_rec_s 	*lrec;

    while (clog_head != clog_tail) {
        lrec= &conn_list[clog_head];
        if (lrec->data) {
            kfree (lrec->data);
            lrec->data = NULL;
        }

        clog_head++;
        if (clog_head >= max_log) clog_head=0;
    }

    conn_count=0;

}

void clear_filters(void)
{
    filter_rec_s	*frec;

    while (filter_count >0 ) {
        frec = &filter_list[filter_count-1];
        if (frec->data) {
            kfree (frec->data);
            frec->data = NULL;
        }

        filter_count--;
    }

}

void reset_filter_counters(void)
{
    filter_rec_s	*frec;
    u32 fcount= filter_count;

    while (fcount >0 ) {
        frec = &filter_list[fcount-1];
        frec->hitcount=0;
        fcount--;
    }
}

int add_filter(u32 hits,u32 act, u16 port, char *m_host, char *m_req, char *m_all, char *m_new)
{
    filter_rec_s	*frec;
    char		*dptr = NULL;
    int		mlen=0;

    if (filter_count >= max_filters) return -ENOMEM;

    frec = &filter_list[filter_count];

    if (m_host) mlen = mlen + strlen(m_host) + 1;
    if (m_req)  mlen = mlen + strlen(m_req)  + 1;
    if (m_all)  mlen = mlen + strlen(m_all)  + 1;
    if (m_new)  mlen = mlen + strlen(m_new)  + 1;

    if (mlen >0) {
        dptr = kmalloc(mlen, GFP_KERNEL);
        if (! dptr) return -ENOMEM;
        }

    frec->id	= filter_count+1;
    frec->data	= dptr;
    frec->action 	= act;
    frec->port	= ((port >> 8) & 0xFF) | ((port << 8) & 0xFF00);
    frec->hitcount	= hits;
    frec->match_all = NULL;
    frec->match_req = NULL;
    frec->match_host= NULL;
    frec->match_new= NULL;

    if ((m_host!= NULL) && (strlen(m_host) >0)) {
        strcpy(dptr,m_host);
        frec->match_host = dptr;
        dptr = dptr + strlen(m_host) + 1;
    }
    if ((m_req!= NULL) && (strlen(m_req) >0)) {
        strcpy(dptr,m_req);
        frec->match_req = dptr;
        dptr = dptr + strlen(m_req) + 1;
    }
    if ((m_all!= NULL) && (strlen(m_all) >0)) {
        strcpy(dptr,m_all);
        frec->match_all = dptr;
        dptr = dptr + strlen(m_all) + 1;
        
        if ( (m_new!= NULL) && (strlen(m_all) == strlen(m_new)) ) {
            strcpy(dptr,m_new);
            frec->match_new = dptr;
            dptr = dptr + strlen(m_new) + 1;
        }
    }

    filter_count++;

    return 0;
}

filter_rec_s *check_filters(struct tcphdr *tcph, char *data, u32 data_len)
{
    char *ss_all = data;
    u32 ss_len = data_len;
    char *ss_host= NULL;
    char *ss_req = NULL;
    filter_rec_s *frec;
    char *repl;
    u32 flt_no=0;
    int strings_extracted = 0;
    int host_matched,req_matched,all_matched;

    if (!data) return NULL;

    while (flt_no < filter_count ) {
        frec = &filter_list[flt_no];
        if (tcph->dest == frec->port) {
            host_matched=req_matched=all_matched=0;

            if ( !frec->match_host && !frec->match_req && !frec->match_all) {
                frec->hitcount++;
                return frec;
                }
            
            if (strings_extracted == 0) {

                while (data != NULL &&  data_len >= 5) {

                    if (strings_extracted == 0) {
                        if (strnicmp(data,"GET " ,4) == 0 || 
                            strnicmp(data,"PUT " ,4) == 0 ||
                            strnicmp(data,"POST ",5) == 0 ||
                            strnicmp(data,"HEAD ",5) == 0 )  ss_req  = data;
                    }else{
                        if (strnicmp(data,"Host:",5)==0) ss_host = strip_leading_spaces(data+5);
                    }

                    while (*data != '\r' && *data != '\n' && data_len > 0 ) {
                        data++;
                        data_len--;
                    }
                    while ( (*data == '\r' || *data == '\n') && data_len > 0 ) {
                        data++; 
                        data_len--;
                        }
                   strings_extracted++;
                }
            }

            if (frec->match_host) {
                if ( (ss_host) && ( strstr_nl(ss_host, frec->match_host) ) ) host_matched = 1;
            } else host_matched=1;

            if (frec->match_req) {
                if ( (ss_req) && ( strstr_nl(ss_req, frec->match_req) ) ) req_matched = 1;
            } else req_matched=1;

            if (frec->match_all) {
                if ( (ss_all) && ( repl=strNstr(ss_all, frec->match_all,ss_len) ) ) {
                    all_matched = 1;
                    if (  frec->match_new  ) memcpy(repl,frec->match_new,strlen(frec->match_new));
                }
            } else all_matched=1;

            if ( (all_matched+req_matched+host_matched)==3 ) {
                frec->hitcount++;
                return frec;
            }
        }

        flt_no++;
    }

    return NULL;
}

int process_tcp_packet(struct tcphdr *tcph, struct iphdr *iph, struct sk_buff *sb)
{
    u32 tcplen;
    filter_rec_s *frec = NULL;
    log_rec_s *lrec = &conn_list[clog_tail];
    char *data;

    if (master_bypass != 0 ) return NF_ACCEPT;

    data = (char *) tcph + (tcph->doff << 2);
    tcplen = (u32) ntohs(iph->tot_len) - (iph->ihl << 2) - (tcph->doff << 2);


    if (tcplen <=0 ) return NF_ACCEPT;

    frec=check_filters(tcph, data, tcplen);

    if ( !frec ) return NF_ACCEPT;

    lrec->matched = frec->id;
    lrec->ctime= current_kernel_time();
    lrec->saddr= iph->saddr;
    lrec->daddr= iph->daddr;
    lrec->sport= tcph->source;
    lrec->dport= tcph->dest;
    lrec->protocol= iph->protocol;
    lrec->urg = tcph->urg;
    lrec->ack = tcph->ack;
    lrec->psh = tcph->psh;
    lrec->rst = tcph->rst;
    lrec->syn = tcph->syn;
    lrec->fin = tcph->fin;
    lrec->datalen= tcplen;
    lrec->action= frec->action;
    if (frec->match_new) lrec->action |= ACT_ALTER;

    if (lrec->data) {
        kfree (lrec->data);
        lrec->data= NULL;
    }

    if (tcplen > max_data) tcplen = max_data;
    lrec->data = kmalloc(tcplen+1, GFP_KERNEL);

    if (! lrec->data) return NF_ACCEPT;

    memcpy(lrec->data,data,tcplen);
    lrec->data[tcplen] = 0;

    conn_count++;
    lrec->conn_id = conn_count;

    clog_tail++;
    if (clog_tail >= max_log) clog_tail=0;
    if (clog_tail == clog_head) {
        clog_head++;
        if (clog_head >= max_log) clog_head=0;
    }

    if ( (frec->action & ACT_DROP) != 0 ) {
        if (clear_data !=0) memset(data,0,tcplen);

        tcph->fin =1;
        tcph->rst =1;
        tcph->psh =0;
    }

    if ( (frec->action & 0xFF) != ACT_IGNORE ) { 

        tcplen=	ntohs(iph->tot_len) - iph->ihl *4;

        tcph->check = 0;
        tcph->check = tcp_v4_check(tcplen, iph->saddr,iph->daddr, csum_partial(tcph, tcplen, 0));

        iph->check = 0;
        ip_send_check(iph);
    }

    return NF_ACCEPT;
}

static unsigned int hook_func(unsigned int hooknum,
                              struct sk_buff *skb, const struct net_device *in,
                              const struct net_device *out, int (*okfn)(struct sk_buff *))
{
    struct sk_buff *sb = skb;
    struct tcphdr *tcph;
    struct iphdr *iph =(struct iphdr *)skb_network_header(sb);

    if (!sb ) return NF_ACCEPT;
    if (!iph) return NF_ACCEPT;

    if (iph->protocol != IPPROTO_TCP) return NF_ACCEPT;

    tcph = (struct tcphdr *)(skb_network_header(sb) + ip_hdrlen(sb));

    return process_tcp_packet(tcph,iph,sb); 

}

int getLogCount(void) {

    int cnt = clog_tail-clog_head;
    if (cnt >=0) return cnt;

    return max_log + 1 + cnt;
    
}

static void *nfhook_seq_start(struct seq_file *s, loff_t *pos)
{

    if ( *pos == 0 )
    {

        switch (api_cmd) {
        case API_GET_PACKET:
            break;
        case API_GET_LOG:
            if (getLogCount()==0) return NULL;
            log_pkt_req=clog_head;
            break;
        case API_GET_FILTERS:
            if (filter_count==0) return NULL;
        default:
            break;
        }

        return pos;
    }
    else
    {

        switch (api_cmd) {
        case API_GET_FILTERS:
            if ( *pos < filter_count) return pos;
            break;
        case API_GET_LOG:
            if ( *pos < getLogCount()) return pos;
            break;
        default:
            break;
        }

        (*pos) = 0;
        return NULL;
    }
}

static void *nfhook_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    u32 *recno = (u32 *)v;
    
    switch (api_cmd) {
    case API_GET_FILTERS:
        (*pos)++;
        (*recno)++;
        if (*recno < filter_count) return recno;
        break;
    case API_GET_LOG:
        (*pos)++;
        (*recno)++;
        if (*recno < getLogCount()) return recno;
        break;
    }

    return NULL;
}

static void nfhook_seq_stop(struct seq_file *s, void *v)
{
}

static int nfhook_seq_show(struct seq_file *s, void *v)
{
    log_rec_s *lrec;
    u32 recno = *(u32 *)v;
    u32 *rptr = (u32*)v;
    char flags[8];
    int fcount=0, ret=0;

    filter_rec_s *frec;

    switch (api_cmd) {
    case API_GET_VERSION:
        return seq_printf(s, "API%s %u %u %u %u %u %u\n", MY_API_VERSION, max_log, max_filters, max_data, conn_count, filter_count, master_bypass);
    case API_GET_FILTERS:
            frec = &filter_list[recno];
            ret= seq_printf(s, "%u\t%u\t%u\t%u\t%s\t%s\t%s\t%s\n", frec->id, frec->hitcount,
                       frec->action,frec->port,frec->match_host,frec->match_req,frec->match_all,frec->match_new);
            if ((ret !=0)&& (recno >0)) (*rptr)--;
        break;
    case API_GET_PACKET:
        lrec= &conn_list[log_pkt_req];
        if (lrec->data) {
            for (fcount=0;fcount < lrec->datalen; fcount++) {
                if  ( (lrec->data[fcount] == '\n') ||
                        ( (lrec->data[fcount] != '\\')&& (lrec->data[fcount] <= 126) && (lrec->data[fcount] >= ' ')))
                    ret=seq_putc(s, lrec->data[fcount]);
                else ret=seq_printf(s, "\\%02X",lrec->data[fcount]);
                if (ret !=0) return ret;
            }
        }
        break;

    case API_GET_LOG:
        recno += log_pkt_req;
        while (recno >=max_log) recno-=max_log;

        lrec= &conn_list[recno];

        if (lrec->urg) flags[fcount++]='U';
        if (lrec->ack) flags[fcount++]='A';
        if (lrec->psh) flags[fcount++]='P';
        if (lrec->rst) flags[fcount++]='R';
        if (lrec->syn) flags[fcount++]='S';
        if (lrec->fin) flags[fcount++]='F';
        if (fcount == 0) flags[fcount++]='-';
        flags[fcount]=0;

        ret= seq_printf(s, "%lu.%03lu %u %u %d.%d.%d.%d:%u %d.%d.%d.%d:%u %u %u %s\n",
                   lrec->ctime.tv_sec,
                   lrec->ctime.tv_nsec/1000000,
                   recno, lrec->matched,
                   NIPQUAD(lrec->saddr),ntohs(lrec->sport),NIPQUAD(lrec->daddr),ntohs(lrec->dport),lrec->datalen, lrec->action, flags);
        if ((ret !=0)&& (recno >0)) (*rptr)--;

        break;
    }

    return ret;
}

int split_by_tab(char *str, char *ptrs[], int max) {
    int cnt=1;
    ptrs[0] = str;

    while( ( *str != 0) && (cnt <max) ) {
        if ( *str == '\t') {
            *str = 0;
            str++;
            ptrs[cnt++] = str;
        }else{
            str++;
        }
    }

    return cnt;
}

static ssize_t nfhook_write(struct file *file, const char *wbuf, size_t len, loff_t * off)
{
    char buffer[IOBUF_SIZE+1];

    u8 acmd;
    unsigned int pktno, mswitch;
    u32 fid,fhits,fact,fport;
    char *sptrs[5];

    if (len <3) return len;

    if ( len > IOBUF_SIZE )	len = IOBUF_SIZE;

    if ( copy_from_user(buffer, wbuf, len) ) {
        return -EFAULT;
    }

    buffer[len]=0;

    if ( (buffer[0] == 'R') && (buffer[1] == 'Q')) {
        acmd = buffer[2] - '0';
        switch (acmd) {
        case API_GET_PACKET:
            if ( (sscanf(&buffer[3],"%u",&pktno) == 1) && (pktno < max_log) ) {
                api_cmd = acmd;
                log_pkt_req = pktno;
            }
            break;
        case API_MASTER_TOGGLE:
            if ( (sscanf(&buffer[3],"%u",&mswitch) == 1) ) master_bypass = (mswitch & 1);
            break;
        case API_SET_FILTER:
            if (len <12) return len;
            
            if (split_by_tab(&buffer[3],sptrs,5) != 5) return len;
            if ( sscanf(sptrs[0],"%d %d %d %d",&fid,&fhits,&fact,&fport) == 4) {
                add_filter(fhits,fact,fport,sptrs[1],sptrs[2],sptrs[3],sptrs[4]);
                }
            break;
        case API_CLEAR_LOG:
            clear_log();
            break;
        case API_CLEAR_FILTERS:
            clear_filters();
            break;
        case API_RESET_FILTER_COUNTERS:
            reset_filter_counters();
            break;
        default:
            if (acmd < API_LAST_COMMAND) api_cmd = acmd;
            break;
        }

    }

    return len;
}

static struct seq_operations seq_ops = {
    .start = nfhook_seq_start,
    .next  = nfhook_seq_next,
    .stop  = nfhook_seq_stop,
    .show  = nfhook_seq_show
};

static int nfhook_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &seq_ops);
};

static struct file_operations proc_fops = {
    .owner   = THIS_MODULE,
    .open    = nfhook_open,
    .write 	 = nfhook_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = seq_release
};

int init_module()
{
    if ((max_log < 32) || (max_log > 4096)) max_log = MAX_LOG_RECORDS;
    if ((max_data < 32) || (max_data > 8192)) max_data = MAX_DATA_LEN;
    if ((max_filters < 10) || (max_filters > 512)) max_filters = MAX_FILTER_RECORDS;

    conn_list = kmalloc(sizeof(log_rec_s) * max_log, GFP_KERNEL);
    if (! conn_list) return -ENOMEM;

    filter_list = kmalloc(sizeof(filter_rec_s) * max_filters, GFP_KERNEL);
    if (! filter_list)  {
        kfree(conn_list);
        return -ENOMEM;
    }

    memset( (void*)conn_list, 0, sizeof(log_rec_s) * max_log);
    memset( (void*)filter_list, 0, sizeof(filter_rec_s) * max_filters);

    nfho.hook     = hook_func;
    nfho.owner    = THIS_MODULE;
    nfho.pf       = PF_INET;
    nfho.hooknum  = NF_INET_POST_ROUTING;
    nfho.priority = NF_IP_PRI_FIRST;

    nf_register_hook(&nfho);

    proc_create(PROCFS_NAME, 0666, NULL, &proc_fops);

    return 0;
}

void cleanup_module()
{
    remove_proc_entry(PROCFS_NAME, NULL);
    nf_unregister_hook(&nfho);

    clear_filters();
    clear_log();

    if (conn_list) kfree(conn_list);
    if (filter_list) kfree(filter_list);
}
