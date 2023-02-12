#!/usr/bin/python
#-- coding:UTF-8 --
import time
import threading
import copy

# This marks a buffer as continuing via the next field. */
VRING_DESC_F_NEXT = 1
# This marks a buffer as write-only (otherwise read-only). */
VRING_DESC_F_WRITE = 2
# This means the buffer contains a list of buffer descriptors. */
VRING_DESC_F_INDIRECT = 4

class Pkts(object):
    pkts=["a","b","c","d","e", "g", "h", "i", "k"]
    pkts_iterator = None

    def __init__(self): 
        self.pkts_iterator = self.process_pkt(self.pkts)

    def process_pkt(self, pkts):
        for pkt in pkts:
            yield pkt

    def gen_pkts(self):
        pkt = ""
        try:
            pkt = self.pkts_iterator.next()
        except:
            self.pkts_iterator = self.process_pkt(self.pkts)
            #pkt = copy.deepcopy(self.pkts_iterator.next())
            pkt = self.pkts_iterator.next()
        return pkt

class Desc(object):
    addr = 0
    next = 0
    flag = 0
    

class VringDesc(object):
    desc_len = 0
    loop_flag = 0
    vr_desc = []
    def __init__(self, desc_len=10):
        self.desc_len = desc_len
        for i in range(0, self.desc_len):
            self.vr_desc.append(Desc())
            self.vr_desc[i].next = i + 1

    def display_vr_desc(self):
        print("addr:")
        for i in range(0, self.desc_len): 
            print(self.vr_desc[i].addr)
        print("next:")
        for i in range(0, self.desc_len): 
            print(self.vr_desc[i].next)

class VringAvail(object):
	avail_idx = 0
	ring = []

class VringUsed(object):
	used_idx = 0;
	ring = [];

class Vring(object):
    desc = VringDesc()
    avail = VringAvail()
    used = VringUsed()
    skb_data = []
    ring_size = desc.desc_len
    free_num = desc.desc_len
    free_head = 0
    last_avail_idx = 0
    last_used_index = 0
    drop_count = 0
    put_count = 0
    get_count = 0
    tx_loop_count = 0
    rx_loop_count = 0
    def __init__(self):
        for i in range(0, self.ring_size):
            self.avail.ring.append(0)
            self.used.ring.append(0)
            self.skb_data.append(0)
            
    def put_ring(self, pkt):
        if self.free_num > 0 and not self.desc.loop_flag:
            buff_index = self.free_head
            self.desc.vr_desc[buff_index].addr = pkt
            self.skb_data[buff_index] = 1
            self.avail.ring[(self.avail.avail_idx )%self.ring_size] = buff_index
            self.avail.avail_idx += 1
            self.free_num -= 1
            self.put_count += 1
            self.free_head = self.desc.vr_desc[buff_index].next
            if self.free_num == 0:
                self.desc.loop_flag = 1
            print("free_head:%d"%(self.free_head))
        else:
            self.drop_count += 1
            print("vring is full")

    def get_ring(self):
        pkts = []
        avail_idx = self.avail.avail_idx
        if self.free_num < self.ring_size:
            if self.desc.loop_flag:
                pkts_buff_indexs = self.ring_size
            else:   
                pkts_buff_indexs = (avail_idx - self.last_avail_idx)%self.ring_size

            print("avail_idx:%d last_avail_idx:%d"%(avail_idx, self.last_avail_idx))
            for i in range(0, pkts_buff_indexs):
                buff_index = self.avail.ring[(self.last_avail_idx + i)%self.ring_size]
                pkts.append(self.desc.vr_desc[buff_index].addr)
                self.get_count += 1
                print("buff_index:%d"%(buff_index))
                self.used.ring[(self.used.used_idx)%self.ring_size] = buff_index
                print("self.used.ring:%d"%(self.used.ring[(self.used.used_idx)%self.ring_size]))
                self.used.used_idx = (self.used.used_idx + 1)%self.ring_size
            self.last_avail_idx = avail_idx
            if self.desc.loop_flag:
                self.desc.loop_flag = 0
            return pkts
        else:
            print("vring is empty")

    def detach_buf(self, head):
        i = head
        self.skb_data[head] = 0
        if self.desc.vr_desc[head].flag == VRING_DESC_F_NEXT:
            i = self.desc.vr_desc[head].next
            self.free_num += 1
        print("detach_buf:%d"%(head))
        self.desc.vr_desc[i].next =  self.free_head 
        self.free_num += 1
        self.free_head = head

    def kick(self):
        pass

    def free_used_index(self):
        used_index = self.used.used_idx
        #如何解决两种套圈场景 1. 后端卡死: used_index一直和last_used_index相等
        #                    2. 后端未卡死但是used_index涨的太快，导致used_index 套圈前面的last_used_index
        print("used_index:%d last_used_index:%d self.free_num:%d"%(used_index, self.last_used_index, self.free_num))
        if self.free_num == 0 and used_index == self.last_used_index and self.desc.loop_flag == 0:
            for i in range(0, self.ring_size):
                buff_index = self.used.ring[(self.last_used_index + i)%self.ring_size]
                self.detach_buf(buff_index)
            
        free_index = (used_index - self.last_used_index)%self.ring_size
        for i in range(0, free_index):
            buff_index = self.used.ring[(self.last_used_index + i)%self.ring_size]
            self.detach_buf(buff_index)

        self.last_used_index = used_index

def produce(vring):
    pkt_gen = Pkts()
    while True:
        time.sleep(0.1)
        pkt = pkt_gen.gen_pkts()
        vring.free_used_index()
        vring.put_ring(pkt)
        print("vring put_count:%d drop_count:%d "%(vring.put_count, vring.drop_count))

def consume(vring):
    while True:
        time.sleep(1)
        pkts = vring.get_ring()
        print("consume pkts:" + str(pkts))

def virtio_test_start():
    vring = Vring()
    t1 = threading.Thread(target=produce, args=(vring,))
    t2 = threading.Thread(target=consume, args=(vring,))
    t1.start()
    t2.start()
    t1.join()
    t2.join()     

if __name__ == "__main__":
    virtio_test_start()