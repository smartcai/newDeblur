/* putmpg.c, 
 *����˶������������
 *   */



#include <stdio.h>
#include "global.h"

/* Ϊ֡�ڱ��������䳤���� */
void putintrablk(blk,cc)
short *blk;
int cc;
{
  int n, dct_diff, run, signed_level;

  /* DC ϵ�� */
  dct_diff = blk[0] - dc_dct_pred[cc]; /* ��ǰһ��Ĳ�ֵ */
  dc_dct_pred[cc] = blk[0];

  if (cc==0)
    putDClum(dct_diff);
  else
    putDCchrom(dct_diff);

  /* AC ϵ�� */
  run = 0;
  for (n=1; n<64; n++)
  {
    /* ʹ��һ�����ʵ��ر���ɨ�跽ʽ*/
    signed_level = blk[(altscan ? alternate_scan : zig_zag_scan)[n]];
    if (signed_level!=0)
    {
      putAC(run,signed_level,intravlc);
      run = 0;
    }
    else
      run++; /* ��0ϵ������ */
  }

  /* ����һ���飬�����ϱ�׼���ս�����*/ 
  if (intravlc)
    putbits(6,4); /* 0110  (�� B-15)  */
  else
    putbits(2,2); /* 10 10 (�� B-14) */
}

/* Ϊ��֡�ڱ��������䳤���� */
void putnonintrablk(blk)
short *blk;
{
  int n, run, signed_level, first;

  run = 0;
  first = 1;

  for (n=0; n<64; n++)
  {
     /* ʹ��һ�����ʵ��ر���ɨ�跽ʽ*/
    signed_level = blk[(altscan ? alternate_scan : zig_zag_scan)[n]];

    if (signed_level!=0)
    {
      if (first)
      {
        /* ��֡�ڿ�ĵ�һ��ϵ�� */
        putACfirst(run,signed_level);
        first = 0;
      }
      else
        putAC(run,signed_level,0);

      run = 0;
    }
    else
      run++; /* ��0ϵ������ */
  }

  /* ����һ���飬�����ϱ�׼���ս�����*/ 
  putbits(2,2);
}

/* Ϊ�˶����������䳤���� */
void putmv(dmv,f_code)
int dmv,f_code;
{
  int r_size, f, vmin, vmax, dv, temp, motion_code, motion_residual;

  r_size = f_code - 1; /* �̶����ȱ���ı����� */
  f = 1<<r_size;
  vmin = -16*f; /* ���� */
  vmax = 16*f - 1; /* ����*/
  dv = 32*f;

  if (dmv>vmax)
    dmv-= dv;
  else if (dmv<vmin)
    dmv+= dv;

  /* ���ֵ�Ƿ���ȷ */
  if (dmv<vmin || dmv>vmax)
    if (!quiet)
      fprintf(stderr,"invalid motion vector\n");

  /* ��dmv�ֳ�dmvmotion_code��motion_residual */
  temp = ((dmv<0) ? -dmv : dmv) + f - 1;
  motion_code = temp>>r_size;
  if (dmv<0)
    motion_code = -motion_code;
  motion_residual = temp & (f-1);

  putmotioncode(motion_code); /* �䳤���� */

  if (r_size!=0 && motion_code!=0)
    putbits(motion_residual,r_size); /* �������� */
}