int main()
{
    int x, y;
    int tmp;
    while (y != 0)
    {
        tmp = x % y;
        x = y;
        y = tmp;
    }
    char a;
    if(a == 'b'){
        printf("Hello World");
    }
    return 0;
}
