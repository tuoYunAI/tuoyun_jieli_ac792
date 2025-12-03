/*****************************************************************************/
/* Montage Technology (Shanghai) Co., Ltd                                    */
/* MONTAGE PROPRIETARY AND CONFIDENTIAL                                      */
/* Copyright (c) 2016 Montage Technology Group Limited. All Rights Reserved. */
/*****************************************************************************/
/*
 * Filename:        mt_fe_i2c_tc6930.c
 *
 * Description:     2-wire bus functions for FE module.
 *
 * Author:          YZ.Huang
 *
 * Version:         1.01.00
 * Date:            2014-12-23
 * History:
 * Description      Version     Date            Author
 *----------------------------------------------------------------------------
 * File Create      1.01.00     2012.02.28      YZ.Huang
 *      Modify      1.03.03     2016.04.14      YZ.Huang
 *----------------------------------------------------------------------------
 *****************************************************************************/

#include "mt_fe_common_tc6930.h"
#include "mt_fe_def_tc6930.h"
#include "device/iic.h"

// About TC6930 Debug I2C

#define TC6930_ID_W				0xAC // TC6930's I2C debug port slave ID.
#define TC6930_ID_R				0XAD

// Read several bytes
// chipAddr	 ---- Slave chip address of I2C device
// pData	 ---- Pointer to store the read data
// ulLength	 ---- Bytes to read
int I2cRead(U8 chipAddr, U8 *pData, U32 ulLength)
{
    int i;
    void *iic = dev_open("iic3", NULL);
    if (!iic) {
        printf("iic open err\n");
        return FALSE;
    }
    dev_ioctl(iic, IOCTL_IIC_START, 0);//IIC锁定
    dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, chipAddr);//起始信号 + 读数据命令
    delay(1000);
    for (i = 0; i < (ulLength - 1); i++) {
        dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_ACK, pData);//读数据 + ACK
        pData++;
        delay(1000);
    }
    dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_STOP_BIT, pData);//读数据 + 结束信号
    delay(1000);
    dev_ioctl(iic, IOCTL_IIC_STOP, 0);//IIC解锁
    return TRUE;
}

// Write ulLength bytes
// chipAddr	 ---- Slave chip address of I2C device
// pData	 ---- Pointer of data to be written
// ulLength	 ---- Bytes to read
int I2cWrite(U8 chipAddr, U8 *pData, U32 ulLength)
{
    int i;
    void *iic = dev_open("iic3", NULL);
    if (!iic) {
        printf("iic open err\n");
        return FALSE;
    }
    dev_ioctl(iic, IOCTL_IIC_START, 0);//IIC锁定
    dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, chipAddr);//写地址
    delay(1000);
    for (i = 0; i < (ulLength - 1); i++) {
        dev_ioctl(iic, IOCTL_IIC_TX_BYTE, *pData); //写数据 + 结束信号
        pData++;
        delay(1000);
    }
    dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, *pData); //写数据 + 结束信号
    delay(1000);

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);//IIC解锁
    return TRUE;
}

// Read 32-bit data from 32-bit address
U32 TC6930_ReadChipReg32(U32 RegAddress)
{
    U8	ReadData[4];
    U8	WriteData[5];
    U8	ucByte;
    U32	unResult;
    //RegAddress &= 0x1fffffff;
    // Special flag for write operation.
    WriteData[0] = 0x1e;
    // Write address, MSB first.
    ucByte = (U8)(RegAddress >> 24);
    WriteData[1] = ucByte;
    ucByte = (U8)(RegAddress >> 16);
    WriteData[2] = ucByte;
    ucByte = (U8)(RegAddress >> 8);
    WriteData[3] = ucByte;
    ucByte = (U8)(RegAddress >> 0);
    WriteData[4] = ucByte;
    //_mt_i2c_tx_with_stop(SLAVE_ID, WriteData, 5, 0);
    I2cWrite(TC6930_ID_W, WriteData, 5);

    // Read the data out.

    //_mt_i2c_rx_with_stop(SLAVE_ID, ReadData, 4, 0);
    I2cRead(TC6930_ID_R, ReadData, 4);
    // Return.
    unResult  = 0xff000000 & (((U32)ReadData[0]) << 24);
    unResult |= 0x00ff0000 & (((U32)ReadData[1]) << 16);
    unResult |= 0x0000ff00 & (((U32)ReadData[2]) << 8);
    unResult |= 0x000000ff & (((U32)ReadData[3]) << 0);
    return unResult;
}
// Write 32-bit data to 32-bit address

void TC6930_WriteChipReg32(U32 RegAddress, U32 WData)
{
    U8	WriteData[9];
    U8	ucByte;
    //RegAddress &= 0x1fffffff;
    // Special flag for write operation.
    WriteData[0] = 0x1f;
    // Write address, MSB first.
    ucByte = (U8)(RegAddress >> 24);
    WriteData[1] = ucByte;
    ucByte = (U8)(RegAddress >> 16);
    WriteData[2] = ucByte;
    ucByte = (U8)(RegAddress >> 8);
    WriteData[3] = ucByte;
    ucByte = (U8)(RegAddress >> 0);
    WriteData[4] = ucByte;
    // Write data.
    ucByte = (U8)(WData >> 24);
    WriteData[5] = ucByte;

    ucByte = (U8)(WData >> 16);
    WriteData[6] = ucByte;
    ucByte = (U8)(WData >> 8);
    WriteData[7] = ucByte;
    ucByte = (U8)(WData >> 0);
    WriteData[8] = ucByte;

    I2cWrite(TC6930_ID_W, WriteData, 9);

    return;
}

/*****************************************************************
** Function: _mt_fe_dmd_set_reg
**
**
** Description:	write data to demod register
**
**
** Inputs:
**
**	Parameter		Type		Description
**	----------------------------------------------------------
**	reg_index		U8			register index
**	data			U8			value to write
**
**
** Outputs:
**
**
*****************************************************************/

MT_FE_RET _mt_fe_dmd_set_reg_tc6930(void *dev_handle, U8 demod_index, U8 reg_index, U8 data)
{
    MT_FE_TC6930_Device_Handle handle = (MT_FE_TC6930_Device_Handle)dev_handle;

    /*
    	TODO:
    		Obtain the i2c mutex
    */

    /*
    	TODO:
    		write data to demodulator register
    */

    if (demod_index == 0) {
        TC6930_WriteChipReg32(0x003b000 + reg_index * 4, data);
    } else if (demod_index == 1) {
        TC6930_WriteChipReg32(0x003c000 + reg_index * 4, data);
    }

    /*
    	TODO:
    		Release the i2c mutex
    */

    return MtFeErr_Ok;
}

/*****************************************************************
** Function: _mt_fe_dmd_get_reg
**
** Description:	read data from demod register
**
** Inputs:
**
**	Parameter		Type		Description
**	----------------------------------------------------------
**	reg_index		U8			register index
**
**
**	Parameter		Type		Description
**	----------------------------------------------------------
**	p_buf			U8*			register data
**
**
*****************************************************************/
MT_FE_RET _mt_fe_dmd_get_reg_tc6930(void *dev_handle, U8 demod_index, U8 reg_index, U8 *p_buf)
{
    MT_FE_TC6930_Device_Handle handle = (MT_FE_TC6930_Device_Handle)dev_handle;
    U32 ulTmp = 0;

    /*
    	TODO:
    		Obtain the i2c mutex
    */

    /*
    	TODO:
    		read demodulator register value
    */
    if (demod_index == 0) {
        ulTmp = TC6930_ReadChipReg32(0x003b000 + reg_index * 4);
    } else if (demod_index == 1) {
        ulTmp = TC6930_ReadChipReg32(0x003c000 + reg_index * 4);
    }
    *p_buf = (U8)ulTmp;

    /*
    	TODO:
    		Release the i2c mutex
    */

    return MtFeErr_Ok;

}



MT_FE_RET _mt_fe_tn_write_tc6930(void *dev_handle, U8 iIndex, U8 *p_buf, U16 n_byte)
{
    MT_FE_TC6930_Device_Handle handle = (MT_FE_TC6930_Device_Handle)dev_handle;
    MT_FE_RET	ret = MtFeErr_Ok;
    if (handle == NULL) {
        return MtFeErr_Uninit;
    }

    /*
    	TODO:
    		Obtain the i2c mutex
    */

    /*
    	TODO:
    		write N bytes to tuner
    */

    /*
    	TODO:
    		Release the i2c mutex
    */

    return MtFeErr_Ok;
}

MT_FE_RET _mt_fe_write32_tc6930(U32 reg_addr, U32 reg_data)
{
    return MtFeErr_Ok;
}

MT_FE_RET _mt_fe_read32_tc6930(U32 reg_addr, U32 *p_data)
{
    return MtFeErr_Ok;
}

MT_FE_RET _mt_fe_dsp_get_reg_tc6930(U8 iIndex, U8 reg_addr, U8 *reg_data)
{
    U32 ulTmp;
    /*
    	TODO:
    		Obtain the i2c mutex
    */

    /*
    	TODO:
    		read tuner register value
    */

    if (iIndex == 0) {
        ulTmp = TC6930_ReadChipReg32(0x0038000 + reg_addr * 4);
    } else if (iIndex == 1) {
        ulTmp = TC6930_ReadChipReg32(0x0039000 + reg_addr * 4);
    } else {
        ulTmp = TC6930_ReadChipReg32(0x003a000 + reg_addr * 4);
    }

    *reg_data = (U8)ulTmp;
    /*
    	TODO:
    		Release the i2c mutex
    */

    return MtFeErr_Ok;

}

S32 _mt_fe_dsp_set_reg_tc6930(U8 iIndex, U8 reg_addr, U8 reg_data)
{
    /*
    	TODO:
    		Obtain the i2c mutex
    */

    /*
    	TODO:
    		write value to tuner register
    */
    if (iIndex == 0) {
        TC6930_WriteChipReg32(0x0038000 + reg_addr * 4, reg_data);
    } else if (iIndex == 1) {
        TC6930_WriteChipReg32(0x0039000 + reg_addr * 4, reg_data);
    } else {
        TC6930_WriteChipReg32(0x003a000 + reg_addr * 4, reg_data);
    }

    /*
    	TODO:
    		Release the i2c mutex
    */

    return MtFeErr_Ok;
}
S32 _mt_fe_tn_get_reg_16bit_tc6930(U16 reg_addr, U16 *reg_data)
{
    U32 ulTmp;
    /*
    	TODO:
    		Obtain the i2c mutex
    */

    /*
    	TODO:
    		read tuner register value
    */

    ulTmp = TC6930_ReadChipReg32(reg_addr * 4 + 0x20000);
    *reg_data = (U16)ulTmp;
    /*
    	TODO:
    		Release the i2c mutex
    */

    return 0;
}

S32 _mt_fe_tn_set_reg_16bit_tc6930(U16 reg_addr, U16 reg_data)
{
    /*
    	TODO:
    		Obtain the i2c mutex
    */
    /*
    	TODO:
    		write value to tuner register
    */
    TC6930_WriteChipReg32(reg_addr * 4 + 0x20000, reg_data);

    /*
    	TODO:
    		Release the i2c mutex
    */

    return MtFeErr_Ok;
}

S32 _mt_fe_tc6930_get_reg_32bit(U32 reg_addr, U32 *reg_data)
{
    /*
    	TODO:
    		Obtain the i2c mutex
    */

    /*
    	TODO:
    		read tuner register value
    */

    *reg_data = TC6930_ReadChipReg32(reg_addr * 4 + 0x20000);

    /*
    	TODO:
    		Release the i2c mutex
    */

    return 0;
}

S32 _mt_fe_tc6930_set_reg_32bit(U32 reg_addr, U32 reg_data)
{
    /*
    	TODO:
    		Obtain the i2c mutex
    */

    /*
    	TODO:
    		write value to tuner register
    */

    TC6930_WriteChipReg32(reg_addr * 4 + 0x20000, reg_data);

    /*
    	TODO:
    		Release the i2c mutex
    */
    return MtFeErr_Ok;
}

S32 _mt_fe_tc6930_set_reg_32bit_bits(U32 ulAddr, U32 ulData, U8 ucBitHigh, U8 ucBitLow)

{

    U32 ulTmp = 0, ulVal = 0;



    U8 ucTmp;



    if ((ucBitHigh > 31) || (ucBitLow > 31))

    {

        return -1;

    }



    if (ucBitHigh < ucBitLow)

    {

        ucTmp = ucBitHigh;

        ucBitHigh = ucBitLow;

        ucBitLow = ucTmp;

    }



    ulData <<= (31 + ucBitLow - ucBitHigh);

    ulData &= 0xFFFFFFFF;

    ulData >>= (31 - ucBitHigh);

    ulData &= 0xFFFFFFFF;



    ulTmp = 0xFFFFFFFF;

    ulTmp <<= (31 + ucBitLow - ucBitHigh);

    ulTmp &= 0xFFFFFFFF;

    ulTmp >>= (31 - ucBitHigh);
    ulTmp &= 0xFFFFFFFF;

    _mt_fe_tc6930_get_reg_32bit(ulAddr, &ulVal);
    ulVal &= ~ulTmp;
    ulVal |= ulData;
    _mt_fe_tc6930_set_reg_32bit(ulAddr, ulVal);
    return 0;
}

void _mt_sleep_tc6930(U32 ticks_ms)

{
    mdelay(ticks_ms);
}


