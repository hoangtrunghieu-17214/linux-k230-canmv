/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Copyright (c) 2023, Canaan Bright Sight Co., Ltd
 * Copyright (C) 2024, Hoang Trung Hieu <hoangtrunghieu.gch17214@gmail.com>
 */

#ifndef K230_CONSTANTS_H
#define K230_CONSTANTS_H

#define K230_PLL_DIVIDE_OUT_SHIFT 24
#define K230_PLL_DIVIDE_OUT_MASK GENMASK(3, 0)
#define K230_PLL_DIVIDE_REF_SHIFT 16
#define K230_PLL_DIVIDE_REF_MASK GENMASK(5, 0)
#define K230_PLL_DIVIDE_FB_SHIFT 0
#define K230_PLL_DIVIDE_FB_MASK GENMASK(12, 0)

#define K230_PLL_DIVIDE_BYPASS_REG_ENABLE_BIT 19
#define K230_PLL_GATE_REG_ENABLE_BIT 2
#define K230_PLL_GATE_REG_WRITE_ENABLE_BIT    18

#define K230_PLL_LOCK_REG_STATUS_BIT 0


// Offsets relative to the PLL clock register address
#define K230_PLL_CLK_DIVIDE_OFFSET 0
#define K230_PLL_CLK_DIVIDE_BYPASS_OFFSET 4
#define K230_PLL_CLK_GATE_OFFSET 8
#define K230_PLL_CLK_LOCK_OFFSET 0xC

// A table that maps a clock rate to the correct mul/div values
struct k230_fracdiv_table {
	u32 rate;
	u32 mul;
	u32 div;
};

struct k230_pll_clk {
        struct clk_hw hw;

        void __iomem *reg;
	spinlock_t *pll_lock;
};

#define to_k230_pll_clk(_hw) container_of(_hw, struct k230_pll_clk, hw)

struct k230_clk_fracdiv {
	struct clk_hw hw;
	const struct k230_fracdiv_table *fracdiv_table;
	// Some clocks have the numerator and denominator values on different registers
	void __iomem *reg, *reg_1;
	spinlock_t *lock;

	u32 m_shift;
	u32 m_mask;
	u32 m_min;
	u32 m_max;
	u32 n_shift;
	u32 n_mask;
	u32 n_min;
	u32 n_max;

	u8 bit_idx;
};

#define to_k230_clk_fracdiv(_hw) container_of(_hw, struct k230_clk_fracdiv, hw)

struct k230_internal_osc_cfg {
	const char *name;
	u32 onecell_idx;
	u32 freq;
};

struct k230_pll_clk_cfg {
	const char *name;
	u32 onecell_idx;
	u32 pll_reg_offset;
};

struct k230_fixedfactor_clk_cfg {
	const char *name, *parent_name;
	u32 onecell_idx;
	u32 clk_div; // mult is always 1
};

struct k230_composite_clk_cfg {
	const char *name;

	u32 onecell_idx;
	const char * const * parent_names;
	int num_parents;

	bool is_fracdiv;
	bool is_fracdiv_1;
	bool is_gate;
	bool is_mux;

	u32 fracdiv_reg_offset;
	u32 fracdiv_reg_1_offset;
	u32 gate_reg_offset;

	u32 fracdiv_m_min;
	u32 fracdiv_m_max;
	u32 fracdiv_n_min;
	u32 fracdiv_n_max;

	u32 fracdiv_m_shift;
	u32 fracdiv_m_mask;
	u32 fracdiv_n_shift;
	u32 fracdiv_n_mask;

	u32 fracdiv_write_enable_bit;
	u8 gate_enable_bit;
	bool gate_is_inverse;

	u32 mux_reg_offset;
	u8 mux_shift;
	u32 mux_mask;
};

#define K230_INTERNAL_OSC(_onecell_idx, _name, _freq) { 			\
	.onecell_idx = _onecell_idx,                  				\
        .name = _name,                         					\
	.freq = _freq								\
}

#define K230_PLL(                                                             	\
	_onecell_idx, _name, _reg_offset                                        \
) { 										\
	.onecell_idx = _onecell_idx, 						\
	.name = _name, 								\
	.pll_reg_offset = _reg_offset 						\
}

#define K230_FIXEDFACTOR( 							\
	_onecell_idx, _name, _parent_name, _div 				\
) { 										\
	.onecell_idx = _onecell_idx, 						\
	.name = _name, 								\
	.parent_name = _parent_name, 						\
	.clk_div = _div 							\
}

#define K230_GATE( 								\
	_onecell_idx, _name, _parent_name, 					\
	_gate_reg_offset, _gate_enable_bit, _gate_is_inverse 			\
) { 										\
	.onecell_idx = _onecell_idx, 						\
	.name = _name, 								\
	.parent_names = (const char *[]){ _parent_name }, 			\
	.num_parents = 1,         						\
                           							\
	.is_gate = true,                                         		\
	.gate_reg_offset = _gate_reg_offset,			 		\
	.gate_enable_bit = _gate_enable_bit,                     		\
	.gate_is_inverse = _gate_is_inverse                      		\
}

#define K230_DIV( 								\
	_onecell_idx, _name, _parent_name, 					\
	_fracdiv_reg_offset,                                                    \
	_fracdiv_m_min, _fracdiv_m_max, _fracdiv_m_shift, _fracdiv_m_mask,      \
	_fracdiv_n_min, _fracdiv_n_max, _fracdiv_n_shift, _fracdiv_n_mask,      \
	_fracdiv_write_enable_bit                                               \
) { 										\
	.onecell_idx = _onecell_idx, 						\
	.name = _name,					 			\
	.parent_names = (const char *[]){ _parent_name }, 			\
	.num_parents = 1, 							\
										\
	.is_fracdiv = true,                                                     \
	.fracdiv_reg_offset = _fracdiv_reg_offset,                              \
										\
	.fracdiv_m_min = _fracdiv_m_min,                                      	\
	.fracdiv_m_max = _fracdiv_m_max,                                      	\
	.fracdiv_m_shift = _fracdiv_m_shift,                                  	\
	.fracdiv_m_mask = _fracdiv_m_mask,                                    	\
										\
	.fracdiv_n_min = _fracdiv_n_min,                                      	\
	.fracdiv_n_max = _fracdiv_n_max,                                      	\
	.fracdiv_n_shift = _fracdiv_n_shift,                                  	\
	.fracdiv_n_mask = _fracdiv_n_mask,                                    	\
										\
	.fracdiv_write_enable_bit = _fracdiv_write_enable_bit                   \
}

#define K230_GDIV( 								\
	_onecell_idx, _name, _parent_name, 					\
	_gate_reg_offset, _gate_enable_bit, _gate_is_inverse, 			\
	_fracdiv_reg_offset,                                                    \
	_fracdiv_m_min, _fracdiv_m_max, _fracdiv_m_shift, _fracdiv_m_mask,      \
	_fracdiv_n_min, _fracdiv_n_max, _fracdiv_n_shift, _fracdiv_n_mask,      \
	_fracdiv_write_enable_bit                                               \
) { 										\
	.onecell_idx = _onecell_idx, 						\
	.name = _name, 								\
	.parent_names = (const char *[]){ _parent_name }, 			\
	.num_parents = 1, 							\
										\
	.is_fracdiv = true,                                                     \
	.fracdiv_reg_offset = _fracdiv_reg_offset,                              \
										\
	.fracdiv_m_min = _fracdiv_m_min,                                      	\
	.fracdiv_m_max = _fracdiv_m_max,                                      	\
	.fracdiv_m_shift = _fracdiv_m_shift,                                  	\
	.fracdiv_m_mask = _fracdiv_m_mask,                                    	\
										\
	.fracdiv_n_min = _fracdiv_n_min,                                      	\
	.fracdiv_n_max = _fracdiv_n_max,                                      	\
	.fracdiv_n_shift = _fracdiv_n_shift,                                  	\
	.fracdiv_n_mask = _fracdiv_n_mask,                                    	\
										\
	.fracdiv_write_enable_bit = _fracdiv_write_enable_bit,                  \
										\
	.is_gate = true,                                         		\
	.gate_reg_offset = _gate_reg_offset,					\
	.gate_enable_bit = _gate_enable_bit,                     		\
	.gate_is_inverse = _gate_is_inverse                      		\
}

#define K230_GDIV1( 									\
	_onecell_idx, _name, _parent_name, 						\
	_gate_reg_offset, _gate_enable_bit, _gate_is_inverse, 				\
	_fracdiv_reg_offset, _fracdiv_reg_1_offset,                                     \
	_fracdiv_m_min, _fracdiv_m_max, _fracdiv_m_shift, _fracdiv_m_mask,       	\
	_fracdiv_n_min, _fracdiv_n_max, _fracdiv_n_shift, _fracdiv_n_mask,      	\
	_fracdiv_write_enable_bit                                                  	\
) { 											\
	.onecell_idx = _onecell_idx, 							\
	.name = _name, 									\
	.parent_names = (const char *[]){ _parent_name }, 				\
	.num_parents = 1, 								\
											\
	.is_fracdiv = true,                                                       	\
	.fracdiv_reg_offset = _fracdiv_reg_offset,                                   	\
											\
	.fracdiv_m_min = _fracdiv_m_min,                                         	\
	.fracdiv_m_max = _fracdiv_m_max,                                         	\
	.fracdiv_m_shift = _fracdiv_m_shift,                                     	\
	.fracdiv_m_mask = _fracdiv_m_mask,                                       	\
											\
	.fracdiv_n_min = _fracdiv_n_min,                                         	\
	.fracdiv_n_max = _fracdiv_n_max,                                         	\
	.fracdiv_n_shift = _fracdiv_n_shift,                                     	\
	.fracdiv_n_mask = _fracdiv_n_mask,                                       	\
											\
	.fracdiv_write_enable_bit = _fracdiv_write_enable_bit,                        	\
											\
	.is_fracdiv_1 = true,                                                     	\
	.fracdiv_reg_1_offset = _fracdiv_reg_1_offset,                               	\
											\
	.is_gate = true,                                         			\
	.gate_reg_offset = _gate_reg_offset,					 	\
	.gate_enable_bit = _gate_enable_bit,                     			\
	.gate_is_inverse = _gate_is_inverse                      			\
}

#define K230_GMUX( 								\
	_onecell_idx, _name, _parent_names, 					\
	_gate_reg_offset, _gate_enable_bit, _gate_is_inverse, 			\
	_mux_reg_offset, _mux_shift, _mux_mask 					\
) { 										\
	.onecell_idx = _onecell_idx, 						\
	.name = _name, 								\
	.parent_names = _parent_names, 						\
	.num_parents = ARRAY_SIZE(_parent_names), 				\
       										\
	.is_gate = true,                                         		\
	.gate_reg_offset = _gate_reg_offset,					\
	.gate_enable_bit = _gate_enable_bit,                     		\
	.gate_is_inverse = _gate_is_inverse,                      		\
										\
	.is_mux = true,                        					\
	.mux_reg_offset = _mux_reg_offset,     					\
	.mux_shift = _mux_shift,               					\
	.mux_mask = _mux_mask                  					\
}

#define K230_GMD( 								\
	_onecell_idx, _name, _parent_names, 					\
	_gate_reg_offset, _gate_enable_bit, _gate_is_inverse, 			\
	_mux_reg_offset, _mux_shift, _mux_mask,					\
	_fracdiv_reg_offset,                                                    \
	_fracdiv_m_min, _fracdiv_m_max, _fracdiv_m_shift, _fracdiv_m_mask,      \
	_fracdiv_n_min, _fracdiv_n_max, _fracdiv_n_shift, _fracdiv_n_mask,      \
	_fracdiv_write_enable_bit                                               \
) {                       							\
	.onecell_idx = _onecell_idx, 						\
	.name = _name, 								\
	.parent_names = _parent_names, 						\
	.num_parents = ARRAY_SIZE(_parent_names), 				\
       										\
	.is_gate = true,                                         		\
	.gate_reg_offset = _gate_reg_offset,					\
	.gate_enable_bit = _gate_enable_bit,                     		\
	.gate_is_inverse = _gate_is_inverse,                      		\
										\
	.is_mux = true,                        					\
	.mux_reg_offset = _mux_reg_offset,     					\
	.mux_shift = _mux_shift,               					\
	.mux_mask = _mux_mask,  						\
                          							\
	.is_fracdiv = true,                                                     \
	.fracdiv_reg_offset = _fracdiv_reg_offset,                              \
										\
	.fracdiv_m_min = _fracdiv_m_min,                                        \
	.fracdiv_m_max = _fracdiv_m_max,                                        \
	.fracdiv_m_shift = _fracdiv_m_shift,                                    \
	.fracdiv_m_mask = _fracdiv_m_mask,                                      \
     										\
	.fracdiv_n_min = _fracdiv_n_min,                                        \
	.fracdiv_n_max = _fracdiv_n_max,                                        \
	.fracdiv_n_shift = _fracdiv_n_shift,                                    \
	.fracdiv_n_mask = _fracdiv_n_mask,                                      \
										\
	.fracdiv_write_enable_bit = _fracdiv_write_enable_bit                   \
}

#endif //K230_CONSTANTS_H
