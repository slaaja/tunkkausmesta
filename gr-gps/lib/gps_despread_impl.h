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

#ifndef INCLUDED_GPS_GPS_DESPREAD_IMPL_H
#define INCLUDED_GPS_GPS_DESPREAD_IMPL_H

#include <gps/gps_despread.h>

namespace gr {
  namespace gps {

    class gps_despread_impl : public gps_despread
    {
     private:
      // Nothing to declare in this block.
		 
		 
		 // code generator
		 int g0_state[10];
		 int g1_state[10];	 
		 
		 int g1_tap0;
		 int g1_tap1;
		 int code_selection;
		 
		 int osr_int;
		 int osr_counter;
		 
		 int code_sync_counter;
		 int delay_selection;
		 
		 gr_complex codegen_out;
		 gr_complex codegen_out_d;
		 gr_complex codegen_out_dd;
		 
		 
		 // search
		 int search_enabled;
		 int search_avg_counter;
		 int search_avg_selection;
		 int search_avg_accumulator;
		 int search_counter;
		 
		 int best_delay;
		 float best_power;
		 
		 // sample accumulators
		 gr_complex output_sample;
		 gr_complex output_sample_early;
		 gr_complex output_sample_late;

     public:
      gps_despread_impl();
      ~gps_despread_impl();

      // Where all the action really happens
      int work(int noutput_items,
	       gr_vector_const_void_star &input_items,
	       gr_vector_void_star &output_items);
    };

  } // namespace gps
} // namespace gr

#endif /* INCLUDED_GPS_GPS_DESPREAD_IMPL_H */

