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
      : gr::sync_decimator("gps_despread",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex)), osr_in*1023)
    {
    	// reset delay counter
		code_sync_counter = 0;
		delay_selection = 0;
		
		// default to PRN 1
		g1_tap0=1;
		g1_tap1=5;
		code_selection=1;
			
		// reset the registers
		int i = 0;
		
		for(i = 0; i < 10; ++i)
		{
			g0_state[i] = 1;
			g1_state[i] = 1;
			
		}
		
		// default to no osr
		osr_int = osr_in;
		osr_counter = 1;
		
		// reset first sample
		//this->output_sample = 0;
		codegen_out = 1;
		codegen_out_d = 1;
		codegen_out_dd = 1;
		
		// reset search params
		
		best_delay = 0;
		best_power = 0;
    }

    /*
     * Our virtual destructor.
     */
    gps_despread_impl::~gps_despread_impl()
    {
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
	}
	
	void gps_despread_impl::reset_lfsr()
	{
		int i = 0;
		
		for(i = 0; i < 10; ++i)
		{
			g0_state[i] = 1;
			g1_state[i] = 1;
			
		}
	}
	
	void gps_despread_impl::set_delay(int d)
	{
		delay_selection = d;
		output_sample = 0;
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
		
		code_selection = c;
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
			this->g1_tap0=0;
			this->g1_tap1=7;
			break;
			
			case 8:
			this->g1_tap0=1;
			this->g1_tap1=8;
			break;
			
			case 9:
			this->g1_tap0=2;
			this->g1_tap1=9;
			break;
			
			case 10:
			this->g1_tap0=1;
			this->g1_tap1=2;
			break;
			
			case 11:
			this->g1_tap0=2;
			this->g1_tap1=3;
			break;
			
			case 12:
			this->g1_tap0=4;
			this->g1_tap1=5;
			break;
			
			case 13:
			this->g1_tap0=5;
			this->g1_tap1=6;
			break;
			
			case 14:
			this->g1_tap0=6;
			this->g1_tap1=7;
			break;
			
			case 15:
			this->g1_tap0=7;
			this->g1_tap1=8;
			break;
			
			case 16:
			this->g1_tap0=8;
			this->g1_tap1=9;
			break;
			
			case 17:
			this->g1_tap0=0;
			this->g1_tap1=3;
			break;
			
			case 18:
			this->g1_tap0=1;
			this->g1_tap1=4;
			break;
			
			case 19:
			this->g1_tap0=2;
			this->g1_tap1=5;
			break;
			
			case 20:
			this->g1_tap0=3;
			this->g1_tap1=6;
			break;
			
			case 21:
			this->g1_tap0=4;
			this->g1_tap1=7;
			break;
			
			case 22:
			this->g1_tap0=5;
			this->g1_tap1=8;
			break;
			
			case 23:
			this->g1_tap0=0;
			this->g1_tap1=2;
			break;
			
			case 24:
			this->g1_tap0=3;
			this->g1_tap1=5;
			break;
			
			case 25:
			this->g1_tap0=4;
			this->g1_tap1=6;
			break;
			
			case 26:
			this->g1_tap0=5;
			this->g1_tap1=7;
			break;
			
			case 27:
			this->g1_tap0=6;
			this->g1_tap1=8;
			break;
			
			case 28:
			this->g1_tap0=7;
			this->g1_tap1=9;
			break;
			
			case 29:
			this->g1_tap0=0;
			this->g1_tap1=5;
			break;
			
			case 30:
			this->g1_tap0=1;
			this->g1_tap1=6;
			break;
			
			case 31:
			this->g1_tap0=2;
			this->g1_tap1=7;
			break;
			
			case 32:
			this->g1_tap0=3;
			this->g1_tap1=8;
			break;
	
			default: // default
			this->code_selection=1;
			this->g1_tap0=1;
			this->g1_tap1=5;
		}
	}
	

	void gps_despread_impl::advance_lfsr(int steps)
	{
		int i = 0,j = 0;
		char g0_state0_next = (g0_state[2] ^ g0_state[9]) & 1;
		char g1_state0_next = (g1_state[1] ^ g1_state[2]
			  			  	^  g1_state[5] ^ g1_state[7] ^ g1_state[9]) & 1;
		int g1_out = 0;
		
		for(i = 0;i < steps;++i)
		{
			for(j = 9; j >= 0; --j)
			{
				if(j == 0)
				{
					g0_state[j] = g0_state0_next;
					g1_state[j] = g1_state0_next;
				}
				else
				{
					g0_state[j] = g0_state[j-1];
					g1_state[j] = g1_state[j-1];
				}
			}
		
		
			this->codegen_out_dd = this->codegen_out_d;
			this->codegen_out_d = this->codegen_out;
		
			g1_out = (g1_state[g1_tap0] ^ g1_state[g1_tap1]) & 1;
			this->codegen_out = (gr_complex) (2.0 * (( (g1_out ^ g0_state[9]) & 1 ) - 0.5));
		}
	}	
	

    int
    gps_despread_impl::work(int noutput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
    {
		int i = 0;
        const gr_complex *in = (const gr_complex *) input_items[0];
        gr_complex *out = (gr_complex *) output_items[0];

		int outputs=0;
		int consumed=0;

        // Do <+signal processing+>
		for(i = 0 ; i < noutput_items*1023*osr_int ; ++i)
		{
			char g1_out = 0;
			
			this->output_sample += in[i] * codegen_out_d ;
			consumed++;
			
			
			if(code_sync_counter == 0)
			{
				// we have now accumulated a whole output sample.
				// spit it out.
				
				out[outputs++] = output_sample;
				
				reset_lfsr(); // the lfsr should already be reset 
							  // but reset it again anyway
				osr_counter = 1;
				
				if(search_enabled)
				{
					search_avg_accumulator += abs(output_sample);
					
					if(search_avg_counter == search_avg_selection)
					{
						search_avg_counter = 1;
						
						if(best_power < search_avg_accumulator)
						{
							best_power = search_avg_accumulator;
							best_delay = delay_selection;
						}
						
						if(search_counter < 1023)
						{
							
							delay_selection += osr_int;
						
							advance_lfsr(1);
						}
						else
						{
							// at this point the LFSR should be at 0 
							// delay
							
							search_enabled = 0;
						
							// TODO: set delay to best delay
							
							advance_lfsr((int)(best_delay/osr_int));
							delay_selection = best_delay;
							
						}
							
					}
					else
					{
						search_avg_counter++;
						
					}
					
					
					
				}
			}
			else if(osr_counter == osr_int)
			{
				this->osr_counter = 1;
				this->advance_lfsr(1);
			}
			else
				osr_counter++;
			
			if(code_sync_counter == ((osr_int * 1023)-1))
				code_sync_counter = 0;
			else
				code_sync_counter++;
			
			
		}
		
		consume_each(consumed);
		
        // Tell runtime system how many output items we produced.
        return outputs;
    }

  } /* namespace gps */
} /* namespace gr */

