#include <stdio.h>


void main(void)
{

int i;

for(i=1; i<=32; i++)
{
 printf("if(isset($_POST[\'dip%02d_v\']))\n",i);
 printf("{\n");
 printf("$fp = fsockopen(\"localhost\", 5963, $errno, $errstr, 30);\n");
     printf("$value = \"V_\" . $_POST[\'dip%02d_v\'];\n",i);
     printf("fwrite($fp,$value);\n");
     printf("fclose($fp);\n");
 printf("$fp = fsockopen(\"localhost\", 5963, $errno, $errstr, 30);\n");
     printf("$value = \"K_DIP%02d_\" . $_POST[\'dip%02d_k\'];\n",i,i);
     printf("fwrite($fp,$value);\n");
     printf("fclose($fp);\n");
 printf("}\n");
}
}
