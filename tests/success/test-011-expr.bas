''------------------------------------------------------------------------------
'' test-011-expr.bas
'' - arithmetic expressions
'' - comparison expressions
'' - implicit conversions
''
'' CHECK: 7, 4, 1
'' CHECK: 7.000000, 4.000000, 1.000000
'' CHECK: true, true, true, false, true
''------------------------------------------------------------------------------
import cstd

dim iseven = 1 + 2 * 3
dim ifour = iseven - 6 / 2
dim ione = 10 mod 3
printf "%d, %d, %d\n", iseven, ifour, ione

dim c3 as double = 3
dim d10 as double = 10
dim dseven = 1 + 2 * c3
dim dfour = dseven - 6 / 2
dim done = d10 mod c3
printf "%lf, %lf, %lf\n", dseven, dfour, done

out "",   iseven > ifour
out ", ", ifour >= ione
out ", ", ione < ifour
out ", ", c3 > dfour
out ", ", iseven = dseven
printf "\n"

sub out(sep as zstring, b as bool)
    printf "%s%s", sep, if b then "true" else "false"
end sub
