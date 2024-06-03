/*
 * i2c_demo.h
 *
 *  Created on: Apr 22, 2024
 *      Author: benyo
 */

#ifndef I2C_DEMO_H_
#define I2C_DEMO_H_

#define I2C_BASE                I2CA0_BASE
#define SYS_CLK                 80000000
#define FAILURE                 -1
#define SUCCESS                 0
#define RETERR_IF_TRUE(condition) {if(condition) return FAILURE;}
#define RET_IF_ERR(Func)          {int iRetVal = (Func); \
                                   if (SUCCESS != iRetVal) \
                                     return  iRetVal;}

int I2CTransact(unsigned long ulCmd);

int I2C_IF_Write(unsigned char ucDevAddr,
         unsigned char *pucData,
         unsigned char ucLen,
         unsigned char ucStop);

int I2C_IF_Read(unsigned char ucDevAddr,
        unsigned char *pucData,
        unsigned char ucLen);
int I2C_IF_ReadFrom(unsigned char ucDevAddr,
            unsigned char *pucWrDataBuf,
            unsigned char ucWrLen,
            unsigned char *pucRdDataBuf,
            unsigned char ucRdLen);

int I2C_IF_Open(unsigned long ulMode);
int I2C_IF_Close();



#endif /* I2C_DEMO_H_ */
