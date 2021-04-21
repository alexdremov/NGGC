LEXEME("def", FDecl)
LEXEME("{", BStart)
LEXEME("}", BEnd)
LEXEME(";", StEnd)
LEXEME("ret", Return)
LEXEME("let", VDecl)
LEXEME("if", If)
LEXEME("else", Else)
LEXEME("while", While)
LEXEME("print", Print)
LEXEME("input", Input)
LEXEME("sin", Sin)
LEXEME("cos", Cos)
LEXEME("tan", Tan)
LEXEME("abs", Abs)
LEXEME("sqrt", Sqrt)
LEXEME("exp", Exp)
LEXEME("setpix", Setpix)

LEXEME("(", LPA)
LEXEME(")", RPA)
LEXEME(",", Comma)

LEXEME("==", Eq)
LEXEME("<=", Leq)
LEXEME(">=", Geq)
LEXEME("!=", Neq)
LEXEME(">", Gr)
LEXEME("<", Le)


LEXEME("=",  Assg)
LEXEME("+=", AdAssg)
LEXEME("-=", MiAssg)
LEXEME("*=", MuAssg)
LEXEME("/=", DiAssg)

LEXEME("+", Plus)
LEXEME("-", Minus)
LEXEME("*", Mul)
LEXEME("/", Div)
LEXEME("^", Pow)

LEXEME("", Identifier)
LEXEME("", Number)
LEXEME("", NumberInt)
LEXEME("", None)