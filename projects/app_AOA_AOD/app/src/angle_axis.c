
#include "angle_axis.h"
#include "app.h"
#include "app_task.h"                // Application task Definition

#include "co_math.h"                 // Common Maths Definition

#include "gpio.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h> 


float ABS_DOUBL(float abs)
{
    if(abs >= 0)
    {
        return abs;
    }else
    {
        return -abs;
    }
}


float clac_iq2angle(float i[],float q[],uint16_t len)
{

    float cc = 300000000;
    float freq = 2400000000;
    float lamda  = (cc/freq);
    float dd  = 0.057;//(0.4*lamda);
    float pi = 3.1415926;
    float phi_A1_THD = pi;
    float phi_A0_THD = pi;
    int32_t clac_total = (len - 11) / 4;
    uint32_t valid_num = 0;
    float temp_total_A1 = 0;
    float temp_total_A0 = 0;
    float temp1_total_A0 = 0;
    float phi_A1_mean;
    float phi_A1_valid_mean;
    float phi_A0_mean;
    float phi_A0_valid_mean;
    
    static float deg_valid[10];

    float temp1;
    float temp2;
    float phi_A1[45];
    float phi_A0[45];
    float theta_est = 0;
    float phi_est;
    float phi_est_valid[10];
    float phi_est_valid_tmp;
    float deg;
    static uint8_t phi_deg_valid_num = 0;
    if(clac_total <= 0)
    {
        return 0;
    }
    uint32_t first_solt = 8 + 5 - 1;
    
  //  uart_printf("clac_total:%d\r\n", clac_total);
    for(int clac_cnt = 0; clac_cnt < clac_total;clac_cnt++)
    {

        temp1 = (atan2(i[first_solt + 1],q[first_solt + 1]) - atan2(i[first_solt],q[first_solt])); // 6 - 5
        temp2 = (atan2(i[first_solt],q[first_solt]) - atan2(i[first_solt - 1],q[first_solt - 1])); // 5 - 4
        
        
        phi_A1[clac_cnt]  =(temp1 - temp2);
        
      //  uart_printf("A1 temp1:%f,temp2:%f,temp3:%f\r\n\r\n", temp1,temp2,phi_A1[clac_cnt]);
        if(phi_A1[clac_cnt] > pi)
        {
            phi_A1[clac_cnt] = phi_A1[clac_cnt] - 2 * pi;
        }
        if(phi_A1[clac_cnt]  < -pi)
        {
            phi_A1[clac_cnt] = phi_A1[clac_cnt] + 2 * pi;
        }
        temp_total_A1 += phi_A1[clac_cnt];
        first_solt+= 4;
    }
      
    phi_A1_mean = temp_total_A1 /clac_total ;
    
    
    temp_total_A1 = 0;
    //**********remove A1 unvalid data*****//
    for(uint32_t clac_valid_num = 0; clac_valid_num < clac_total;clac_valid_num++)
    {
        if(ABS_DOUBL(phi_A1[clac_valid_num] - phi_A1_mean)  <= phi_A1_THD)
        {
            phi_A1[valid_num] = phi_A1[clac_valid_num];
        //    uart_printf("phi_A1[%d]:%f,abs:%f\r\n\r\n",valid_num, phi_A1[valid_num],ABS_DOUBL(phi_A1[clac_valid_num] - phi_A1_mean));
            temp_total_A1 += phi_A1[valid_num];
            valid_num++;
        }
        
    }
    
    
    phi_A1_valid_mean = temp_total_A1 / valid_num;
    
   // uart_printf("phi_A1_mean:%f,phi_A1_valid_mean:%f,valid_num:%d\r\n\r\n", phi_A1_mean,phi_A1_valid_mean,valid_num);
    
    /*******************************/
    
    
    first_solt = 8 + 4 ;
   
    //***********A0 reference phase deviation***********************///
    for(int clac_cnt = 0; clac_cnt < clac_total;clac_cnt++)
    {

        temp1 = (atan2(i[first_solt + 2],q[first_solt + 2]) - atan2(i[first_solt],q[first_solt])); // 
        temp2 = (atan2(i[first_solt],q[first_solt]) - atan2(i[first_solt - 1],q[first_solt - 1])); //
        phi_A0[clac_cnt]  =(temp1 - temp2);
        
    //    uart_printf("A0 temp1:%f,temp2:%f,temp3:%f\r\n\r\n", temp1,temp2,phi_A0[clac_cnt]);
        if(phi_A0[clac_cnt] > pi)
        {
            phi_A0[clac_cnt] = phi_A0[clac_cnt] - 2 * pi;
        }
        if(phi_A0[clac_cnt]  < -pi)
        {
            phi_A0[clac_cnt] = phi_A0[clac_cnt] + 2 * pi;
        }
        temp_total_A0 += phi_A0[clac_cnt];
        first_solt+= 4;
    }
      
    phi_A0_mean = temp_total_A0 /clac_total ;
    
    temp_total_A0 = 0;
    valid_num = 0;
    //**********remove A0 unvalid data*****//
    for(uint32_t clac_valid_num = 0; clac_valid_num < clac_total;clac_valid_num++)
    {
        if(ABS_DOUBL(phi_A0[clac_valid_num] -phi_A0_mean)  <= phi_A0_THD)
        {
            phi_A0[valid_num] = phi_A0[clac_valid_num];
            temp_total_A0 += phi_A0[valid_num];
            valid_num++;
        }
        
    }
    
    phi_A0_valid_mean = temp_total_A0 / valid_num;
    /*******************************/
    
    
   
    phi_est = phi_A1_valid_mean ;//- phi_A0_valid_mean;
    phi_est_valid_tmp = phi_est*lamda/2/pi/dd;
    
    if(phi_est_valid_tmp > 1)
    {
        phi_est_valid_tmp = 1;
    }
    else if(phi_est_valid_tmp < -1)
    {
        phi_est_valid_tmp = -1;
    }
    
       
    theta_est = acos(phi_est_valid_tmp);
    deg = theta_est*180/pi;
    //uart_printf("deg %5.2f\r\n",deg);
    if((deg >= -180) && (deg <= 180))
    {
        deg_valid[phi_deg_valid_num] = deg;
      //  uart_printf("tt deg_valid[%d] = %5.2f deg\r\n",phi_deg_valid_num,deg_valid[phi_deg_valid_num]);
        phi_deg_valid_num ++;
    }
    
   // uart_printf("Estimated angle of tag2 is %5.2f deg\r\n",deg);
//    
    if(phi_deg_valid_num == 10)
    {
        phi_deg_valid_num = 0;
        temp_total_A0 = 0;
        for(int i = 0; i< 10;i++)
        {
            temp_total_A0 += deg_valid[i];
            //uart_printf("deg_valid[%d] %5.2f deg\r\n",i,deg_valid[i]);
        }
        
        phi_est_valid_tmp =  temp_total_A0 / 10;
        //uart_printf("phi_est_valid_tmp %5.2f deg\r\n",phi_est_valid_tmp);
         //**********remove deg unvalid data*****//
        valid_num = 0;
        
        
        temp_total_A0 = 0;
        for(uint32_t clac_valid_num = 0; clac_valid_num < 10;clac_valid_num++)
        {
            if(ABS_DOUBL(deg_valid[clac_valid_num] - phi_est_valid_tmp)  <= 10)
            {
                phi_A0[valid_num] = deg_valid[clac_valid_num];
                temp_total_A0 += phi_A0[valid_num];
                valid_num++;
            }        
        }       
        //uart_printf("valid avg  %5.2f deg ,valid_num:%d\r\n",temp_total_A0 / valid_num,valid_num);
      
       temp1_total_A0 = 0; 
        for(uint32_t clac_valid_num = 0; clac_valid_num < 10;clac_valid_num++)
        {
            temp1_total_A0 += ABS_DOUBL(deg_valid[clac_valid_num] - phi_est_valid_tmp) * ABS_DOUBL(deg_valid[clac_valid_num] - phi_est_valid_tmp) ;
                 
        }
         //uart_printf("ABS  %5.2f deg\r\n",temp1_total_A0);
        
        if(temp1_total_A0 < 10000)
        {
            
            if(valid_num != 0)
            {
                phi_est_valid_tmp = temp_total_A0 / valid_num;
            }else
            {
            
            }
        
        }
        else
        {
            if(phi_est_valid_tmp > 60 )
            {
                 phi_est_valid_tmp = 170;
            }else
            {
                phi_est_valid_tmp = 10;
            }
           
        }
                 
        
    /*******************************/

        //  uart_printf("A0_valid_mean:%f,A1_valid_mean:%f,p:%f\r\n", phi_A0_mean,phi_A1_mean,phi_est);
        //  uart_printf("A0_valid_mean:%f,A1_valid_mean:%f,p:%f\r\n", phi_A0_valid_mean,phi_A1_valid_mean,phi_est);
       // uart_printf("***********************************\r\n");
       // uart_printf("Estimated avg angle of tag2 is %5.2f deg,valid_num:%d\r\n", phi_est_valid_tmp,valid_num);
        uart_printf("***********************************\r\n\r\n");
        
        deg_gpio_set(phi_est_valid_tmp);
                
    }
    
    
    
   
//    theta_est = acos(phi_est_valid_tmp);
//  //  uart_printf("A0_valid_mean:%f,A1_valid_mean:%f,p:%f\r\n", phi_A0_mean,phi_A1_mean,phi_est);
//  //  uart_printf("A0_valid_mean:%f,A1_valid_mean:%f,p:%f\r\n", phi_A0_valid_mean,phi_A1_valid_mean,phi_est);
//    uart_printf("Estimated phi_est:%f,angle of tag2 is %5.2f deg\r\n",phi_est, theta_est*180/pi);
//    
}


struct axis_t clac_angle2axis(uint8_t num,float angle[],struct axis_t axis[])
{
   // GPIO_DEBUG_TRIGER(0xb);
    struct axis_t XY;
    float pi = 3.1415926;
    
    float a = 0,b = 0,c = 0,c1 = 0,c2 = 0,d = 0,e = 0,f = 0,f1 = 0, f2 = 0,D = 0,D1 = 0,D2 = 0;
    float tmp = 0;
    
    for(uint8_t i = 0;i < num;i++)
    {
        a+= (tan(angle[i])*tan(angle[i]));
        b+=  tan(angle[i]);
        c1 += (axis[i].x * tan(angle[i])*tan(angle[i]));
        c2 += (axis[i].y *tan(angle[i]));
        f1 += (axis[i].y);
        f2 += (axis[i].x *tan(angle[i]));
    }
    a = 2 * a;
    b = -2 * b;
    c = 2 * c1 - 2 * c2;
    d = b;
    e = 2 * num;
    f = 2 * f1 - 2 * f2;
    
    D = a * e - b * d;
    D1 = c * e - b * f;
    D2 = a * f - d * c;
   
    XY.x = D1/D;
    XY.y = D2/D;
    //GPIO_DEBUG_TRIGER(0xb);
    //GPIO_DEBUG_TRIGER(0xb);
    for(uint8_t i = 0;i < num;i++)
    {
        uart_printf("tan(%f):%f,axis[%d].x:%f,axis[%d].y:%f\r\n",180 * angle[i] /pi,tan(angle[i]),i,axis[i].x,i,axis[i].y);
    }
    uart_printf("a:%f,b:%f,c:%f,d:%f,e:%f,f:%f\r\n",a,b,c,d,e,f);
    uart_printf("D:%f,D1:%f,D2:%f\r\n",D,D1,D2);
    
    uart_printf("clac_angle2axis X:%f,y:%f\r\n",XY.x,XY.y);

    return XY;
}


void deg_gpio_init(void)
{
    gpio_config(DEG_0,OUTPUT,PULL_HIGH);
    gpio_config(DEG_30,OUTPUT,PULL_HIGH);
    gpio_config(DEG_60,OUTPUT,PULL_HIGH);
    gpio_config(DEG_90,OUTPUT,PULL_HIGH);
    gpio_config(DEG_120,OUTPUT,PULL_HIGH);
    gpio_config(DEG_150,OUTPUT,PULL_HIGH);
    gpio_config(DEG_180,OUTPUT,PULL_HIGH);
    gpio_set(DEG_0, 0);
    gpio_set(DEG_30, 0);
    gpio_set(DEG_60, 0);
    gpio_set(DEG_90, 0);
    gpio_set(DEG_120, 0);
    gpio_set(DEG_150, 0);
    gpio_set(DEG_180, 0);
}




void deg_gpio_set(float deg)
{    
    gpio_set(DEG_0, 0);
    gpio_set(DEG_30, 0);
    gpio_set(DEG_60, 0);
    gpio_set(DEG_90, 0);
    gpio_set(DEG_120, 0);
    gpio_set(DEG_150, 0);
    gpio_set(DEG_180, 0);
    if((deg > 0) && (deg <= 30))
    {
        gpio_set(DEG_0, 1);
        gpio_set(DEG_30, 1);
    
    }else if((deg >30) && (deg <= 60))
    {
         gpio_set(DEG_30, 1);
        gpio_set(DEG_60, 1);
    
    }else if((deg >60) && (deg <= 90))
    {
        gpio_set(DEG_60, 1);
        gpio_set(DEG_90, 1);
    
    }else if((deg >90) && (deg <= 120))
    {
    
        gpio_set(DEG_90, 1);
        gpio_set(DEG_120, 1);
    }else if((deg >120) && (deg <= 150))
    {
    
        gpio_set(DEG_120, 1);
        gpio_set(DEG_150, 1);
    }else if((deg >150) && (deg <= 181))
    {
        gpio_set(DEG_150, 1);
        gpio_set(DEG_180, 1);
    }
}

