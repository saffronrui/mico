/*
 * vfd.h
 *
 *  Created on: 2018Äê6ÔÂ14ÈÕ
 *      Author: Administrator
 */

#ifndef DEMOS_HELLOWORLD_VFD_H_
#define DEMOS_HELLOWORLD_VFD_H_

#define vfd_char_num	32
#define	vfd_cmd			0xf8
#define	vfd_data		0xfa
#define	vfd_cmd_clear	0x01

int mico_vfd_init();

void	vfd_dis_char(char	addr, char	cont);
void 	vfd_clear(void);


#endif /* DEMOS_HELLOWORLD_VFD_H_ */
