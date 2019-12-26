#include <stdio.h>

int main()
{
    int n,a,num1,num2;
    int x = 0, y = 0;
    int b = 1;
    scanf("%d", &n);
    a = 1;
    while(a <= 9) {
        while (b <= 9)
        {
            num1 = a*10 + b;
            num2 = b*10 + a;
            if(n == num1*num2)
            {
                x = b;
                y = a;

            }
            b = b + 1;
        }
        a = a + 1;
    }
    printf("%d %d", x, y);
    return 0;
}