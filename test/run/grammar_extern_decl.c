/* C part of grammar_extern_decl.abc */

int ext_table[] = { 1, 2, 3, 4 };
int ext_count = 4;
double ext_scale = 0.5;

int
ext_sum(const int *a, int n)
{
    int s = 0;
    for (int i = 0; i < n; ++i) {
	s += a[i];
    }
    return s;
}
