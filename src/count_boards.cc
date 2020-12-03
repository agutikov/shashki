#include <cstdio>
#include <cstdint>
#include <array>

constexpr int count_bits(uint32_t i)
{
     i = i - ((i >> 1) & 0x55555555);
     i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
     return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

int max(int a, int b)
{
    return a > b ? a : b;
}
int min(int a, int b)
{
    return a < b ? a : b;
}

constexpr auto make_hist()
{
    std::array<std::array<int, 32>, 32> hist{{0}};
    size_t m = 0;
    for (uint32_t i = 1; i != 0; i++) {
        int n = count_bits(i);
        if (n <= 1 && i != 1) {
            hist[m+1] = hist[m];
            m++;
        }
        hist[m][n-1]++;
    }
    return hist;
}

void calc_all()
{
    const std::array<std::array<int, 32>, 32> hist = make_hist();

    for (int width = 1; width <= 32; width++) {
        printf("\n-------- width=%d --------\n", width);
        for (int set_bits = 1; set_bits <= 32; set_bits++) {
            printf("%2d : %d\n", set_bits, hist[width-1][set_bits-1]);
        }
    }

    double total = 0;
    for (int occupied = 2; occupied <= 24; occupied++) {
        size_t occ_vars = hist[31][occupied-1];
        printf("\noccupied=%d: %ld * (", occupied, occ_vars);

        bool w_start = true;
        double w_total = 0;
        for (int whites = max(occupied-12, 1); whites <= min(occupied-1, 12); whites++) {

            if (w_start) {
                w_start = false;
            } else {
                printf(" + ");
            }

            size_t w_vars = hist[occupied-1][whites-1];
            printf("%ld * (", w_vars);

            int blacks = occupied - whites;
            bool wk_start = true;
            double wk_total = 0;
            for (int white_kings = 0; white_kings <= whites; white_kings++) {

                if (wk_start) {
                    wk_start = false;
                } else {
                    printf(" + ");
                }

                size_t wk_vars = 1;
                if (white_kings > 0) {
                    wk_vars = hist[whites-1][white_kings-1];
                }
                printf("%ld * (", wk_vars);

                bool bk_start = true;
                double bk_total = 0;
                for (int black_kings = 0; black_kings <= blacks; black_kings++) {

                    if (bk_start) {
                        bk_start = false;
                    } else {
                        printf(" + ");
                    }

                    size_t bk_vars = 1;
                    if (black_kings > 0) {
                        bk_vars = hist[blacks-1][black_kings-1];
                    }
                    printf("%ld", bk_vars);
                    bk_total += bk_vars;
                }
                printf(")");

                wk_total += wk_vars * bk_total;
            }
            printf(")");

            w_total += w_vars * wk_total;
        }
        printf(")\n");

        printf("%lu * %lf = %lf\n", occ_vars, w_total, occ_vars * w_total);
        total += occ_vars * w_total;
        printf("total=%lf\n", total);
    }

    printf("total=%lf\n", total/2);
}


int main()
{
    calc_all();

    return 0;
}
