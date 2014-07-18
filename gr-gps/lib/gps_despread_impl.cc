/* -*- c++ -*- */
/* 
 * Copyright 2014 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "gps_despread_impl.h"

namespace gr {
  namespace gps {

    gps_despread::sptr
    gps_despread::make(int osr_in)
    {
      return gnuradio::get_initial_sptr
        (new gps_despread_impl(osr_in));
    }

    /*
     * The private constructor
     */
    gps_despread_impl::gps_despread_impl(int osr_in)
      : gr::block("gps_despread",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex)))

			  
    {
		
    	// reset delay counter
		delay_selection = 0;
			
		// calcualte some code lut
		set_code(1);
		code_counter = 0;
		
		// default to no osr
		osr_int = osr_in;
		osr_counter = 1;
		
		delay_next = 0;
		
		// reset search params
		search_enabled = 0;
		search_avg_counter = 0;
		search_avg_selection = 0;
		search_avg_accumulator = 0;
		search_counter = 0;
		best_delay = 0;
		best_power = 0;
		
		avg_accu = 0;
		
		// tracking
		tracking_enabled = 0;
    }

	void gps_despread_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required)
	{

		ninput_items_required[0] = (1023)*osr_int*noutput_items;


	}

    /*
     * Our virtual destructor.
     */
    gps_despread_impl::~gps_despread_impl()
    {
    }
	
	float gps_despread_impl::peak() const
	{
		
		return best_power;
	}
	
	int gps_despread_impl::search_running() const
	{
		return search_enabled;
	}
	
	void gps_despread_impl::start_search(int avg)
	{
		search_enabled = 1;
		search_avg_counter = 1;
		search_avg_selection = avg;
		search_avg_accumulator = 0;
		search_counter = 1;
		
		best_delay = 0;
		best_power = 0;
		
		avg_accu = 0;
		
		printf("### Search started \n");
	}
	
	
	
	void gps_despread_impl::set_delay(int d)
	{
		int dd = d % 1023;
		
		if((dd-delay_selection) > 0)
			delay_next = (dd-delay_selection)*osr_int;
		else
			delay_next = (1023-delay_selection+dd)*osr_int;
		
		delay_selection = dd;
		
		//printf("## Delay by %d -> Using %d last samples twice from next sequence\n",dd, delay_next);
	}
	
	int gps_despread_impl::delay() const
	{
		
		return delay_selection;
	}
	
	int gps_despread_impl::osr() const
	{
		return osr_int;
		
	}
	
	void gps_despread_impl::set_osr(int o)
	{
		//this->osr_int = o;
		
	}
	
	int gps_despread_impl::code() const 
	{
		return code_selection;
		
	}

	void gps_despread_impl::set_code(int c)
	{
		int g1_tap0 = 0;
		int g1_tap1 = 0;
		
		code_selection = c;
		
		printf("### Set code to: %d\n", c);
		// code generator and tap coefficients from 
		// Matjaz Vidmar
		// http://lea.hamradio.si/~s53mv/navsats/theory.html
		
		switch(c) 
		{
			case 1:
			g1_tap0=1;
			g1_tap1=5;
			break;
			
			case 2:
			g1_tap0=2;
			g1_tap1=6;
			break;
			
			case 3:
			g1_tap0=3;
			g1_tap1=7;
			break;
			
			case 4:
			g1_tap0=4;
			g1_tap1=8;
			break;
			
			case 5:
			g1_tap0=0;
			g1_tap1=8;
			break;
			
			case 6:
			g1_tap0=1;
			g1_tap1=5;
			break;
			
			case 7:
			g1_tap0=0;
			g1_tap1=7;
			break;
			
			case 8:
			g1_tap0=1;
			g1_tap1=8;
			break;
			
			case 9:
			g1_tap0=2;
			g1_tap1=9;
			break;
			
			case 10:
			g1_tap0=1;
			g1_tap1=2;
			break;
			
			case 11:
			g1_tap0=2;
			g1_tap1=3;
			break;
			
			case 12:
			g1_tap0=4;
			g1_tap1=5;
			break;
			
			case 13:
			g1_tap0=5;
			g1_tap1=6;
			break;
			
			case 14:
			g1_tap0=6;
			g1_tap1=7;
			break;
			
			case 15:
			g1_tap0=7;
			g1_tap1=8;
			break;
			
			case 16:
			g1_tap0=8;
			g1_tap1=9;
			break;
			
			case 17:
			g1_tap0=0;
			g1_tap1=3;
			break;
			
			case 18:
			g1_tap0=1;
			g1_tap1=4;
			break;
			
			case 19:
			g1_tap0=2;
			g1_tap1=5;
			break;
			
			case 20:
			g1_tap0=3;
			g1_tap1=6;
			break;
			
			case 21:
			g1_tap0=4;
			g1_tap1=7;
			break;
			
			case 22:
			g1_tap0=5;
			g1_tap1=8;
			break;
			
			case 23:
			g1_tap0=0;
			g1_tap1=2;
			break;
			
			case 24:
			g1_tap0=3;
			g1_tap1=5;
			break;
			
			case 25:
			g1_tap0=4;
			g1_tap1=6;
			break;
			
			case 26:
			g1_tap0=5;
			g1_tap1=7;
			break;
			
			case 27:
			g1_tap0=6;
			g1_tap1=8;
			break;
			
			case 28:
			g1_tap0=7;
			g1_tap1=9;
			break;
			
			case 29:
			g1_tap0=0;
			g1_tap1=5;
			break;
			
			case 30:
			g1_tap0=1;
			g1_tap1=6;
			break;
			
			case 31:
			g1_tap0=2;
			g1_tap1=7;
			break;
			
			case 32:
			g1_tap0=3;
			g1_tap1=8;
			break;
	
			default: // default
			code_selection=1;
			g1_tap0=1;
			g1_tap1=5;
		}
		
		//printf("tap0: %d, tap2: %d\n", g1_tap0, g1_tap1);

		// calculate our code LUT
		
		char g0[10] = {1,1,1,1,1,1,1,1,1,1};
		char g1[10] = {1,1,1,1,1,1,1,1,1,1};
		char g1_out = 0;
		
		
		int i = 0, j = 0;
		for(i = 0 ; i < 1023 ; ++i)
		{
			g1_out = g1[g1_tap0] ^ g1[g1_tap1];
			
			code_LUT[i] = (gr_complex) (2.0 * ((g1_out ^ g0[9]) - 0.5));
			
			//printf("%d ", g1_out ^ g0[9]);
			
			char g0_state0_next = g0[2] ^ g0[9];
			char g1_state0_next = g1[1] ^ g1[2] ^ g1[5]
					 			^ g1[7] ^ g1[8] ^ g1[9];
			
			for(j = 9; j >= 0; --j)
			{
				if(j == 0)
				{
					g0[j] = g0_state0_next;
					g1[j] = g1_state0_next;
				}
				else
				{
					g0[j] = g0[j-1];
					g1[j] = g1[j-1];
				}
			}
		}
		
		//printf("\n");
		
	}
	

    int
    gps_despread_impl::general_work(int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
		int i = 0;
        const gr_complex *in = (const gr_complex *) input_items[0];
        gr_complex *out = (gr_complex *) output_items[0];

		int outputs=0;
		int consumed=0;


        // Do <+signal processing+>
		for(i = 0 ; i < noutput_items ; ++i)
		{
			int j = 0;
			

			
			code_counter = 0; // always start with fresh chip sequence.
			osr_counter = 1;
			
			output_sample_late = 0;
			output_sample_early = 0;
			output_sample = 0;
			
			//advance_lfsr(1); // match delay
			
			for(j = 0; j < (1023 * osr_int); ++j)
			{
				int counter_early;
				int counter_late;
				// produce a sample with different LFSR phases for tracking (in case we have freq error)
				if(code_counter == 0)
				{
					counter_early = 1;
					counter_late = 1022;
				}
				else if(code_counter == 1022)
				{
					counter_early = 0;
					counter_late = 1021;
				}
				else
				{
					counter_early = code_counter + 1;
					counter_late = code_counter - 1;
					
				}
					
				
				output_sample_early += in[consumed] * code_LUT[counter_early] / (float)osr_int ;
				output_sample += in[consumed] * code_LUT[code_counter] / (float)osr_int ;
				output_sample_late += in[consumed] * code_LUT[counter_late] / (float)osr_int ;
				
				//printf("%d: %f & %f = %f -> %f\n",j, in[consumed].real(),code_LUT[code_counter].real(), (in[consumed] * code_LUT[code_counter]).real(), output_sample.real());
				
				if(delay_next > 0 && j > (1023*osr_int-delay_next-1))
					delay_next--; 
				else
					consumed++;
				
				if(osr_counter == osr_int)
				{
					osr_counter = 1;
					
					if(code_counter == 1022)
						code_counter = 0;
					else
						code_counter++;
					
				}
				else
					osr_counter++;
				
				
			}
				
			
			out[i] = output_sample;
			
			if(search_enabled)
			{
				search_avg_accumulator += out[i];
				
				if(search_avg_counter == search_avg_selection)
				{
					
					if(abs(search_avg_accumulator) > best_power)
					{
						best_power = abs(search_avg_accumulator);
						best_delay = delay_selection;
						
						//printf("## New best power: %f\n", best_power);
					}
					
					
					
					if(search_counter == 1023)
					{
						search_counter = 1;
						search_enabled = 0;
						
						printf("# Current delay: %d\n", delay_selection);
						
						set_delay(best_delay);
						
						printf("### Search finished. Best delay: %d with power: %f average: %f\n", best_delay, best_power, avg_accu/1023);
						
					}
					else
					{
						search_counter++;
						avg_accu += abs(search_avg_accumulator);
						
						set_delay(delay_selection + 1);
					}
					
					search_avg_accumulator = 0;
					search_avg_counter = 1;
				}
				else
					search_avg_counter++;
				
			}			
			else if(tracking_enabled)
			{
				//fggasdfsadgh
			}
		}
			
		//printf("%d consumed: %d \n",code_counter, consumed);
		consume_each(consumed);
		
        // Tell runtime system how many output items we produced.
        return noutput_items;
    }

  } /* namespace gps */
} /* namespace gr */

