#!/opt/local/bin/python2.7
#!/usr/bin/env python

from gnuradio import gr, gr_unittest
from gnuradio import blocks
from gnuradio.eng_option import eng_option
from optparse import OptionParser
import gps
import goldcode_gen
import osmosdr 
import numpy

SNR = 0.5;

DEFAULT_OSR = 2
DEFAULT_SAMPLE_RATE = DEFAULT_OSR * 1023e3
DEFAULT_RX_FREQ = 1575.42e6

class my_topblock(gr.top_block):
    def invert_sequence(self, s):
        c = []
        
        for i in range(0,len(s)):
            c.append(-s[i])
            
        return c
        
    def get_code_sequence(self, osr):
        # PRN 12
        g1_tap0 = 4
        g1_tap1 = 5
        
        g0 = [1,1,1,1,1,1,1,1,1,1]
        g1 = [1,1,1,1,1,1,1,1,1,1]
        
        c = []
        c1 = [0] * 1023
        
        for i in range(0,1023):
            g1_out = g1[g1_tap0] ^ g1[g1_tap1]
        
            for k in range(0, osr):
                c.append(2.0 * ( (g1_out ^ g0[9])  - 0.5) )
                
            c1[i] = g1_out ^ g0[9]
            
            g0_next = g0[2] ^ g0[9]
            g1_next = g1[1] ^ g1[2] ^ g1[5] ^ g1[7] ^ g1[8] ^ g1[9]
            
            for j in range(9,-1,-1):
                if j == 0:              
                    g0[j] = g0_next
                    g1[j] = g1_next
                    
                else:
                    g0[j] = g0[j-1]
                    g1[j] = g1[j-1]
                    
                
        #print c1
        return c
    
    def produce_data(self, b, osr):
        d = [];
        
        for i in range(0,len(b)):
            if b[i] == 0:
                d.extend(self.invert_sequence(self.get_code_sequence(osr)))
            else:
                d.extend(self.get_code_sequence(osr))
        
        
        return d
        
    def __init__(self,kohinat):
        gr.top_block.__init__(self)
        

        #self.src = goldcode_gen.goldcode_gen();
        #self.src.set_code(12)
        #self.src.set_osr(DEFAULT_OSR)
        #self.src.advance_lfsr(10)
        delay = 123 * DEFAULT_OSR
        data = [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
        
        
        self.src_data = self.produce_data(data * 200, DEFAULT_OSR)
        
        self.src_data = self.src_data[delay:]
        
        noise = numpy.random.randn(len(self.src_data))/SNR/SNR
        
        self.src_data = [x + y for x, y in zip(self.src_data, noise)]
        #src_data = src_data + numpy.random.randn(len(src_data))*1/SNR/2

        self.src = blocks.vector_source_c(self.src_data);
       
        
        self.despread=gps.gps_despread(DEFAULT_OSR)

        self.despread.set_code(12)
       

        #self.despread.start_search(10)
        
        self.connect(self.src, self.despread)


        
        self.srcsink = blocks.vector_sink_c()
        self.connect(self.src, self.srcsink)
        self.sink = blocks.vector_sink_c()
        self.connect(self.despread, self.sink)
        
        self.despread.start_search(4)

def main():
    
    #kohinat = numpy.random.randn(1023 * 2 * DEFAULT_OSR)*1/SNR/SNR

    
    paras = 0
    paras_i = 0
    

    tb=my_topblock([])
    
    
    tb.run()

        
        
    
    #print tehot
    print 'len(tb.sink.data()) =',len(tb.sink.data())
    print 'len(tb.srcsink.data()) =',len(tb.srcsink.data())
    
    
    

if __name__ == '__main__':
    main()