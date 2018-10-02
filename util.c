char *readall(FILE *fp)
{
    slice_t str = {.size=sizeof(char)};
    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        str = append(str, &ch);
    }
    ch = '\0';
    str = append(str, &ch);
    return str.array;
}
