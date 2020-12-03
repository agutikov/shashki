#include <string>
#include <iostream>
#include <vector>

#include <cvc4/cvc4.h>


using namespace std::string_literals;




auto XXX = "\n"
"+---+---+---+---+---+---+---+---+ \n"
"|   |   |   |   |   |   |   | x | \n"
"+---+---+---+---+---+---+---+---+ \n"
"| x |   |   |   |   |   |   |   | \n"
"+---+---+---+---+---+---+---+---+ \n"
"|   | x |   |   |   |   |   |   | \n"
"+---+---+---+---+---+---+---+---+ \n"
"|   |   |   |   |   |   |   |   | \n"
"+---+---+---+---+---+---+---+---+ \n"
"|   |   |   |   |   | x |   |   | \n"
"+---+---+---+---+---+---+---+---+ \n"
"|   |   | x |   |   |   |   |   | \n"
"+---+---+---+---+---+---+---+---+ \n"
"|   | # |   |   |   |   |   |   | \n"
"+---+---+---+---+---+---+---+---+ \n"
"|   |   |   |   |   |   |   |   | \n"
"+---+---+---+---+---+---+---+---+ \n"
""s;

const int _ = 0;
const int o = 1;
const int G = 2;
const int x = -1;
const int M = -2;

const int X[8][8] = {
    {_, x, _, x, _, x, _, x},
    {x, _, x, _, x, _, x, _},
    {_, x, _, x, _, x, _, x},
    {_, _, _, _, _, _, _, _},
    {_, _, _, _, _, _, _, _},
    {o, _, o, _, o, _, o, _},
    {_, o, _, o, _, o, _, o},
    {o, _, o, _, o, _, o, _}
};


using namespace CVC4;



void cvc4_test()
{
    ExprManager em;

    Expr helloworld = em.mkVar("Hello World!", em.booleanType());

    SmtEngine smt(&em);

    std::cout << helloworld << " is " << smt.query(helloworld) << std::endl;
}

void cvc4_linar()
{
    ExprManager em;
    SmtEngine smt(&em);

    Type real = em.realType();
    Type integer = em.integerType();

    Expr x = em.mkVar("x", integer);
    Expr y = em.mkVar("y", real);

    Expr three = em.mkConst(Rational(3));
    Expr neg2 = em.mkConst(Rational(-2));
    Expr two_thirds = em.mkConst(Rational(2,3));

    Expr three_y = em.mkExpr(kind::MULT, three, y);
    Expr diff = em.mkExpr(kind::MINUS, y, x);
 
    Expr x_geq_3y = em.mkExpr(kind::GEQ, x, three_y);
    Expr x_leq_y = em.mkExpr(kind::LEQ, x, y);
    Expr neg2_lt_x = em.mkExpr(kind::LT, neg2, x);

    Expr assumptions = em.mkExpr(kind::AND, x_geq_3y, x_leq_y, neg2_lt_x);
    smt.assertFormula(assumptions);

    smt.push();
    Expr diff_leq_two_thirds = em.mkExpr(kind::LEQ, diff, two_thirds);
    std::cout << "Prove that " << diff_leq_two_thirds << " with CVC4." << std::endl;
    std::cout << "CVC4 should report VALID." << std::endl;
    std::cout << "Result from CVC4 is: " << smt.query(diff_leq_two_thirds) << std::endl;
    smt.pop();


}



void cvc4_draughts_2()
{
    ExprManager em;
    SmtEngine smt(&em);

    


}





int main()
{
    cvc4_test();

    cvc4_linar();

    return 0;
}


