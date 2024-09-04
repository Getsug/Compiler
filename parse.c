#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUF_SIZE 256
#define MAX_TOKEN 40
#define TRUE 1
#define FALSE 0
#define W_SIZE 10
#define RESERVE_SIZE 6
#define TRUE 1
#define FALSE 0


/***********************************************/
/***************** Scanning ********************/
/***********************************************/


typedef enum
{
    START, INNUM, INID, INASSIGN, EQ, LT, LTEQ, GT, GTEQ, NOT, NEQ, DIVIDE, INCOMMENT,
    ENDCOMMNET, DONE
}StateType;


typedef enum
{
    /*special symbols*/
    PLUS, MINUS, MULT, SEMICOLON, COMMA, LPAREN, RPAREN, LBRACKET, RBRACKET,
    LCURLY, RCURLY,
    /*reserved words*/
    IF, ELSE, INT, RETURN, VOID, WHILE,
    /*multicharacter tokens*/
    ID, NUM,
    ERROR, ENDOFFILE,
    ASSIGN, EQUAL, LTHAN, LEQ, GTHAN,GEQ, NT, NE, DIV,
    SKIP
}TokenType;


typedef struct TOKEN
{
    /*store string from buffer*/
    char line[BUF_SIZE];
    /*position of next line to read*/
    int line_number;
    /*length of string  stored in line*/
    int line_len;
    /*index of next character to read */
    int character_loc;
    char current_token[MAX_TOKEN];
    /*indicates the position to store a character in current_token*/
    int token_index;

    TokenType tokenType;
}TOKEN;


typedef struct
{
    char word[W_SIZE];
    TokenType tokType;
}Reserved;


/***********************************************/
/***************** Parsing *********************/
/***********************************************/
#define MAXCHILDREN 3

typedef enum {StmtKind, ExpKind} NodeKind;
typedef enum {CompStmtKind, SelectStmtKind, IterStmtKind, ReturnStmtKind, CallKind} StatementKind;
typedef enum {VarDeclKind, VarArrayDeclKind, FuncDeclKind, AssignKind, OpKind, IdKind, ConstKind} ExpressionKind;

typedef enum {Void, Integer} ExpressionType;

//struct scopeListRec;

typedef struct treeNode
{
    struct treeNode *child[MAXCHILDREN];
    struct treeNode *sibling;
    int line_num;
    NodeKind nodekind;

    union 
    {
        StatementKind statement;
        ExpressionKind expression;
    }kind;
    union
    {
        TokenType operator;
        int value;
        char *name;
        //struct ScopListRec *scope;
    }attribute;

    ExpressionType exp_type;
    int isParam;
    int arraySize;
}TreeNode;


/*function prototypes*/

/***************** Scanning ********************/

//void getTok(FILE *source, FILE *destination, Reserved *reservedWords);
char nextChar(FILE *source, FILE *destination);
//void printToken(FILE *destination, FILE *source, TokenType tokType);
void getToken(FILE *source, FILE *destination, Reserved *reservedWords);
void printTok(FILE *destination, TokenType tokType);


/***************** Parsing *********************/
TreeNode * declaration_list(void);
TreeNode * declaration(void);
TreeNode * var_declaration(void);
ExpressionType typeSpecifier(void);
TreeNode * params(void);
TreeNode * param_list(ExpressionType exp_type);
TreeNode * param(ExpressionType exp_type);
TreeNode * compound_statement(void);
TreeNode * local_declaration(void);
TreeNode * statement_list(void);
TreeNode * statement(void);
TreeNode * expression_statement(void);
TreeNode * selection_statement(void);
TreeNode * iteration_statement(void);
TreeNode * return_statement(void);
TreeNode * expression(void);
TreeNode * simple_expression(TreeNode *f);
TreeNode * add_expression(TreeNode *f);
TreeNode * term(TreeNode *f);
TreeNode * factor(TreeNode *f);
TreeNode * call(void);
TreeNode * args(void);
TreeNode * args_list(void);
TreeNode * parse(void);
void printTree(TreeNode *syntaxTree);


/*Global variables*/
int EchoSource = FALSE;
int TraceScan = FALSE;
int TraceParse = TRUE;
int Error = FALSE;

//holds current token data
TOKEN token;
FILE *infile;
FILE *outfile;
Reserved *reservedWords;



int main(int argc, char *argv[])
{
    token.line_number = 0;
    token.character_loc = 0;
    token.line_len = 0;
    //token.token_index = 0;

    reservedWords = (Reserved*)malloc(sizeof(Reserved) *RESERVE_SIZE);

    strcpy(reservedWords[0].word, "if");
    reservedWords[0].tokType = IF;
    strcpy(reservedWords[1].word, "else");
    reservedWords[1].tokType = ELSE;
    strcpy(reservedWords[2].word, "int");
    reservedWords[2].tokType = INT;
    strcpy(reservedWords[3].word, "void");
    reservedWords[3].tokType = VOID;
    strcpy(reservedWords[4].word, "return");
    reservedWords[4].tokType = RETURN;
    strcpy(reservedWords[5].word, "while");
    reservedWords[5].tokType = WHILE;


    if(argc != 3)
    {
        printf("Usage: %s <input_file>  <output_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    infile = fopen(argv[1], "r");
    outfile = fopen(argv[2], "w");

    /*while(fgets(token.line, BUF_SIZE - 1, infile))
    {
        printf("%s\n", token.line);
    }*/

    //getToken(infile, outfile, reservedWords);
    //getTok(infile, outfile, reservedWords);

    TreeNode *syntaxTree = parse();

    if(TraceParse)
    {
        fprintf(outfile, "\nSyntax Tree\n");
        printTree(syntaxTree);
    }
    

}


/**************************************************************/
/*These fucntions are used in scanning and returning tokentype*/
/**************************************************************/

/*get the next character from line*/
char nextChar(FILE *source, FILE *destination)
{
    if(!(token.character_loc < token.line_len))
    {
        token.line_number++;

        if(fgets(token.line, BUF_SIZE - 1, source))
        {
            /*write to the outfile*/
            if(EchoSource)
            {
                 fprintf(destination, "%d:  %s", token.line_number, token.line);
            }
           
            token.line_len = strlen(token.line);
            /*reset character location index*/
            token.character_loc = 0;

            return token.line[token.character_loc++];
        }
        else return EOF;
    }
    else
    {
        return token.line[token.character_loc++];
    }
}

/*move to the previous character location*/
void backtrack(void)
{
    token.character_loc--;
}


void getToken(FILE *source, FILE *destination, Reserved *reservedWords)
{
    char c;
    int save;

    //while(c != EOF)
    //{
    StateType state = START;
    token.token_index = 0;

    while(state != DONE)
    {
        save = TRUE;
        c = nextChar(source, destination);
        char saveDivOperator = FALSE;

        switch(state)
        {
            case START:
                if (c == '/')
                {
                    state = DIVIDE;
                    save = FALSE;
                }
                else if(isalpha(c))
                {
                    state = INID;
                }
                else if ((c == ' ' ) || (c == '\t') || (c == '\n' || (c == '\r') || c == '\f'))
                {
                    save = FALSE;
                    //token.tokenType = SKIP;
                }
                else if(isdigit(c))
                {
                    state = INNUM;
                }
                else if(c == '=')
                {
                    state = INASSIGN;
                }
                else if(c == '<')
                {
                    state = LT;
                }
                else if(c == '>')
                {
                    state = GT;
                }
                else if(c == '!')
                {
                    state = NOT;
                }
                else
                {
                    state = DONE;

                    switch (c)
                    {
                        case EOF:
                            token.tokenType = ENDOFFILE;
                            //printf("EOF\n");
                            break;

                        case '(':
                            token.tokenType = LPAREN;
                            break;

                        case ')':
                            token.tokenType = RPAREN;
                            break;

                        case '[':
                            token.tokenType = LBRACKET;
                            break;

                        case ']':
                            token.tokenType = RBRACKET;
                            break;

                        case '{':
                            token.tokenType = LCURLY;
                            break;

                        case '}':
                            token.tokenType = RCURLY;
                            break;

                        case '+':
                            token.tokenType = PLUS;
                            break;

                        case '-':
                            token.tokenType = MINUS;
                            break;

                        case '*':
                            token.tokenType = MULT;
                            break;

                        case ';':
                            token.tokenType = SEMICOLON;
                            break;

                        case ',':
                            token.tokenType = COMMA;
                            break;

                        default:
                            printf("Invalid character: %c\n", c);
                            token.tokenType = ERROR;
                            break;
                    }
                }
                break;

            case DIVIDE:
                if(c == '*')
                {
                    state = INCOMMENT;
                    save = FALSE;
                }
                else
                {
                    backtrack();
                    save = FALSE;
                    state = DONE;
                    saveDivOperator = TRUE;
                }
                break;

            case INCOMMENT:
                if( c == '*')
                {
                    state = ENDCOMMNET;
                    save = FALSE;
                }
                else
                {
                    save = FALSE;
                }
                break;

            case ENDCOMMNET:
                if(c == '/')
                {
                    state = START;
                    save = FALSE;
                    //printf("back to start\n");
                }
                else
                {
                    state = INCOMMENT;
                    save = FALSE;
                }
                break;

            case INID:
                if(!(isalnum(c)))
                {
                    backtrack();
                    save = FALSE;
                    state = DONE;
                    token.tokenType = ID;
                }
                break;

            case INNUM:
                if(!(isdigit(c)))
                {
                    backtrack();
                    save = FALSE;
                    state = DONE;
                    token.tokenType = NUM;
                }
                break;

            case INASSIGN:
                if(c == '=')
                {
                        state = EQ;
                }
                else
                {
                    backtrack();
                    save = FALSE;
                    state = DONE;
                    token.tokenType = ASSIGN;
                }
                break;

            case EQ:
                backtrack();
                save = FALSE;
                state = DONE;
                token.tokenType = EQUAL;
                break;

            case LT:
                if(c == '=')
                {
                    state = LTEQ;
                }
                else
                {
                    backtrack();
                    save = FALSE;
                    state = DONE;
                    token.tokenType = LTHAN;
                }
                break;

            case LTEQ:
                backtrack();
                save = FALSE;
                state = DONE;
                token.tokenType = LEQ;
                break;

            case GT:
                if(c == '=')
                {
                    state = GTEQ;
                }
                else
                {
                    backtrack();
                    save = FALSE;
                    state = DONE;
                    token.tokenType = GTHAN;
                }
                break;

            case GTEQ:
                backtrack();
                save = FALSE;
                state = DONE;
                token.tokenType = GEQ;
                break;

            case NOT:
                if(c == '=')
                {
                state = NEQ;
                }
                else
                {
                backtrack();
                save = FALSE;
                state = DONE;
                token.tokenType = NT;
                }
                break;

            case NEQ:
                backtrack();
                save = FALSE;
                state = DONE;
                token.tokenType = NE;
                break;

            default:
                fprintf(destination, "Scanner error state: %d\n", state);
                state = DONE;
                token.tokenType = ERROR;
                break;


        }

         /*add chracter to current_token*/
        if((save == TRUE) && (token.token_index < MAX_TOKEN))
        {
            token.current_token[token.token_index++] = c;
        }
        else if(saveDivOperator == TRUE && (token.token_index < MAX_TOKEN))
        {
            token.current_token[token.token_index++] = '/';
            token.tokenType = DIV;
        }

        if(state == DONE)
        {
            //printf("%s", token.current_token);
            //printf("TOKEN: %s\n", token.current_token);
            //printf("tok index = %d\n", token.token_index);
            token.current_token[token.token_index] = '\0';
            //printf("TOKEN: %s\n", token.current_token);

            if(token.tokenType == ID)
            {
                for(int i = 0; i < RESERVE_SIZE; i++)
                {
                    if(strcmp(token.current_token, reservedWords[i].word) == 0)
                    {
                        token.tokenType = reservedWords[i].tokType;
                    }
                }
            }
        }

    }

    if(TraceScan)
    {
        printTok(destination, token.tokenType);
    }

}
//}

void printTok(FILE *destination, TokenType tokType)
{
    switch(tokType)
    {
        case IF:
        case ELSE:
        case INT:
        case VOID:
        case RETURN:
        case WHILE:
            fprintf(destination, "\t%d:  reserved word: %s\n", token.line_number, token.current_token);
            break;
        //case ERROR:
            //fprintf(destination, "%d:  ERROR: %s\n", token.line_number, token.line);
            //break;
        case NUM:
            fprintf(destination, "\t%d:  NUM: %s\n", token.line_number, token.current_token);
            break;

        case ID:
            fprintf(destination, "\t%d:  ID: %s\n", token.line_number, token.current_token);
            break;

        case ENDOFFILE:
            fprintf(destination, "%d:  EOF\n", token.line_number);
            break;

        case ERROR:
            fprintf(destination, "\t%d:  %s(ERROR)\n", token.line_number, token.current_token);
            break;

        case LPAREN:
            fprintf(destination, "\t%d:  (\n", token.line_number);
            break;

        case RPAREN:
            fprintf(destination, "\t%d:  )\n", token.line_number);
            break;

        case LBRACKET:
            fprintf(destination, "\t%d:  %s\n", token.line_number, token.current_token);
            break;

        case RBRACKET:
            fprintf(destination, "\t%d:  %s\n", token.line_number, token.current_token);
            break;

        case LCURLY:
            fprintf(destination, "\t%d:  %s\n", token.line_number, token.current_token);
            break;

        case RCURLY:
            fprintf(destination, "\t%d:  %s\n", token.line_number, token.current_token);
            break;

        case PLUS:
            fprintf(destination, "\t%d:  %s\n", token.line_number, token.current_token);
            break;

        case MINUS:
            fprintf(destination, "\t%d:  %s\n", token.line_number, token.current_token);
            break;

        case MULT:
            fprintf(destination, "\t%d:  %s\n", token.line_number, token.current_token);
            break;

        case DIV:
            fprintf(destination, "\t%d:  %s\n", token.line_number, token.current_token);
            break;

        case SEMICOLON:
            fprintf(destination, "\t%d:  %s\n", token.line_number, token.current_token);
            break;

        case COMMA:
            fprintf(destination, "\t%d:  %s\n", token.line_number, token.current_token);
            break;

        case ASSIGN:
            fprintf(destination, "\t%d:  %s\n", token.line_number, token.current_token);
            break;

        case EQUAL:
            fprintf(destination, "\t%d:  %s\n", token.line_number, token.current_token);
            break;

        case LTHAN:
            fprintf(destination, "\t%d:  %s\n", token.line_number, token.current_token);
            break;

        case GTHAN:
            fprintf(destination, "\t%d:  %s\n", token.line_number, token.current_token);
            break;

        case LEQ:
            fprintf(destination, "/t%d:  %s\n", token.line_number, token.current_token);
            break;

        case GEQ:
            fprintf(destination, "/t%d:  %s\n", token.line_number, token.current_token);
            break;

        case NT:
            fprintf(destination, "\t%d:  %s\n", token.line_number, token.current_token);
            break;

        case NE:
            fprintf(destination, "\t%d:  %s\n", token.line_number, token.current_token);
            break;

        default:
            fprintf(destination, "\tUnknown token\n");
    }
}



/*************************************************************/
/*These fucntions are used in parsing and printing parse tree*/
/*************************************************************/

TreeNode *newStatementNode(StatementKind kind)
{
    TreeNode *t = (TreeNode*)malloc(sizeof(TreeNode));

    if(t == NULL)
    {
        fprintf(outfile, "Lack of adequate memory! Error at line %d\n", token.line_number);
        printf("Lack of adequate memory! Error at line %d\n", token.line_number);
    }
    else
    {
        //assign null to child 
        for(int i = 0; i < MAXCHILDREN; i++)
        {
            t->child[i] = NULL;
        }

        t->sibling = NULL;
        t->nodekind = StmtKind;
        t->kind.statement = kind;
        t->line_num = token.line_number;
    }

    return t;
}

TreeNode *newExpressionNode(ExpressionKind kind)
{
    TreeNode *t = (TreeNode*)malloc(sizeof(TreeNode));

    if(t == NULL)
    {
        fprintf(outfile, "Lack of adequate memory! Error at line %d\n", token.line_number);
        printf("Lack of adequate memory! Error at line %d\n", token.line_number);
    }
    else
    {
        //assign null to child 
        for(int i = 0; i < MAXCHILDREN; i++)
        {
            t->child[i] = NULL;
        }

        t->sibling = NULL;
        t->nodekind = ExpKind;
        t->kind.expression = kind;
        t->line_num = token.line_number;
        t->exp_type = Void;
        t->isParam = FALSE;
    }

    return t;
}

char *copyString(char *string)
{
    int len;
    char *tokenValue;

    if(string == NULL)
        return NULL;

    len = strlen(string) + 1;
    tokenValue = (char*)malloc(len);

     if(tokenValue == NULL)
    {
        fprintf(outfile, "Lack of adequate memory! Error at line %d\n", token.line_number);
        printf("Lack of adequate memory! Error at line %d\n", token.line_number);
    }
    else
    {
        strcpy(tokenValue, string);
    }

    return tokenValue;
}


static void syntaxError(char *message)
{
    fprintf(outfile, "\n>>>");
    fprintf(outfile, "Syntax error at line %d: %s", token.line_number, message);
    printf("\n>>> Syntax error at line %d: %s", token.line_number, message);
    Error = TRUE;
}


ExpressionType typeSpecifier(void)
{
    switch (token.tokenType)
    {
    case INT:
        getToken(infile, outfile, reservedWords);
        return Integer;

    case VOID:
        getToken(infile, outfile, reservedWords);
        return Void;

    default:
        syntaxError("unexpected token (type_spec) -> ");
        printTok(outfile, token.tokenType);
        getToken(infile, outfile, reservedWords);
        return Void;
    }
}


static void match(TokenType expected)
{
    if(token.tokenType == expected)
    {
        getToken(infile, outfile, reservedWords);
    }
    else
    {
        syntaxError("unexpected token (match) -> ");
        printTok(outfile, token.tokenType);
        fprintf(outfile,"      "); // 6 space bars
    }
}

TreeNode *declaration_list(void)
{
    TreeNode *t = declaration();
    TreeNode *p = t;

    while(token.tokenType != ENDOFFILE)
    {
        TreeNode *q = declaration();
        if(q != NULL)
        {
            if(t == NULL)
            {
                t = p = q;
            }
            else
            {
                p->sibling = q;
                p = q;
            }
        }
    }

    return t;
}


TreeNode *declaration(void)
{
    TreeNode *t;
    ExpressionType exp_type;
    char *name;

    exp_type = typeSpecifier();
    name = copyString(token.current_token);
    match(ID);

    switch(token.tokenType)
    {
        case SEMICOLON:
            t = newExpressionNode(VarDeclKind);

            if(t != NULL)
            {
                t->attribute.name = name;
                t->exp_type = exp_type;
            }
            match(SEMICOLON);
            break;

        case LBRACKET:
            t = newExpressionNode(VarArrayDeclKind);

            if(t != NULL)
            {
                t->attribute.name = name;
                t->exp_type = exp_type;
            }
            match(LBRACKET);

            if (t != NULL)
            {
                t->arraySize = atoi(token.current_token);
            }
            match(NUM);
            match(RBRACKET);
            match(SEMICOLON);
            break;

        case LPAREN:
            t = newExpressionNode(FuncDeclKind);

            if(t != NULL)
            {
                t->attribute.name = name;
                t->exp_type = exp_type;
            }
            match(LPAREN);


            if(t != NULL)
            {
                t->child[0] = params();
            }
            match(RPAREN);

            if(t != NULL)
            {
                t->child[1] = compound_statement();
            }
            break;

        default:
            syntaxError("unexpected token (declaration) -> ");
            printTok(outfile, token.tokenType);
            getToken(infile, outfile, reservedWords);
            break;
    }
    return t;
}

TreeNode * var_declaration(void)
{
    TreeNode *t;
    ExpressionType exp_type;
    char *name;
    
    exp_type = typeSpecifier();
    name = copyString(token.current_token);
    match(ID);

    switch (token.tokenType)
    {
    case SEMICOLON:
        t = newExpressionNode(VarDeclKind);

        if (t != NULL)
        {
            t->attribute.name = name;
            t->exp_type = exp_type;
        }
        match(SEMICOLON);
        break;

    case LBRACKET:
        t = newExpressionNode(VarArrayDeclKind);

        if (t != NULL)
        {
            t->attribute.name = name;
            t->exp_type = exp_type;
        }
        match(LBRACKET);

        if (t != NULL)
        {
            t->arraySize = atoi(token.current_token);
        }
        match(NUM);
        match(RBRACKET);
        match(SEMICOLON);
        break;

    default:
        syntaxError("unexpected token (var_declaration) -> ");
        printTok(outfile, token.tokenType);
        getToken(infile, outfile, reservedWords);
        break;
    }
    return t;
}


TreeNode * params(void)
{
    ExpressionType exp_type;
    TreeNode *t;

    exp_type = typeSpecifier();

    if (exp_type == Void && token.tokenType == RPAREN)
    {
        t = newExpressionNode(VarDeclKind);
        t->isParam = TRUE;
        t->exp_type = Void;
    }
    else
    {
        t = param_list(exp_type);
    }

    return t;
}

TreeNode *param_list(ExpressionType exp_type)
{
    TreeNode *t = param(exp_type);
    TreeNode *p = t;
    TreeNode * q;

    while(token.tokenType == COMMA)
    {
        match(COMMA);
        q = param(typeSpecifier());

        if(q != NULL)
        {
            if(t == NULL)
            {
                t = p = q;
            }
            else
            {
                p->sibling = q;
                p = q;
            }
        }

    }

    return t;
}


TreeNode * param(ExpressionType exp_type)
{
    TreeNode *t;
    char *name;

    name = copyString(token.current_token);
    match(ID);

    if (token.tokenType == LBRACKET)
    {
        match(LBRACKET);
        match(RBRACKET);

        t = newExpressionNode(VarArrayDeclKind);
    }
    else
    {
        t = newExpressionNode(VarDeclKind);
    }

    if (t != NULL)
    {
        t->attribute.name = name;
        t->exp_type = exp_type;
        t->isParam = TRUE;
    }
    return t;
}


TreeNode * compound_statement(void)
{
    TreeNode *t = newStatementNode(CompStmtKind);
    match(LCURLY);

    t->child[0] = local_declaration();
    t->child[1] = statement_list();
    match(RCURLY);
    return t;
}


TreeNode *local_declaration(void)
{
    TreeNode *t = NULL;
    TreeNode *p;

    if(token.tokenType == INT || token.tokenType == VOID)
    {
        t = var_declaration();
    }

    p = t;

    if(t != NULL)
    {
        while(token.tokenType == INT || token.tokenType == VOID)
        {
            TreeNode *q = var_declaration();

            if(q != NULL)
            {
                if(t == NULL)
                {
                    t = p = q;
                }
                else
                {
                    p->sibling = q;
                    p = q;
                }
            }
        }
    }
    return t;
}


TreeNode *statement_list(void)
{
    TreeNode *t;
    TreeNode *p;

     if(token.tokenType == RCURLY)
    {
        return NULL;
    }

    t = statement();
    p = t;

    while(token.tokenType != RCURLY)
    {
        TreeNode *q;
        q = statement();

        if(q != NULL)
        {
            if(t == NULL)
            {
                t = p = q;
            }
            else
            {
                p->sibling = q;
                p = q;
            }
        }
    }

    return t;

}


/*CODE TO REVIEW*/

TreeNode * statement(void)
{
    TreeNode *t;
    switch (token.tokenType)
    {
    case LCURLY:
        t = compound_statement();
        break;

    case IF:
        t = selection_statement();
        break;

    case WHILE:
        t = iteration_statement();
        break;

    case RETURN:
        t = return_statement();
        break;
    case ID:
    case LPAREN:
    case NUM:
    case SEMICOLON:
        t = expression_statement();
        break;

    default:
        syntaxError("unexpected token (statement) -> ");
        printTok(outfile, token.tokenType);
        getToken(infile, outfile, reservedWords);
        return Void;
    }
    return t;
}



TreeNode *expression_statement(void)
{
    TreeNode *t;
    
    if (token.tokenType == SEMICOLON)
    {
        match(SEMICOLON);
    }
    else if (token.tokenType != RCURLY)
    {
        t = expression();
        match(SEMICOLON);
    }
    return t;
}


TreeNode *selection_statement(void)
{
    TreeNode *t = newStatementNode(SelectStmtKind   );

    match(IF);
    match(LPAREN);

    if(t != NULL)
    {
        t->child[0] = expression();
    }
    match(RPAREN);

    if(t != NULL)
    {
        t->child[1] = statement();
    }

    if (token.tokenType == ELSE)
    {
        match(ELSE);

        if(t != NULL)
        {
            t->child[2] = statement();
        }
    }
    
    return t;
}


TreeNode * iteration_statement(void)
{
    TreeNode *t = newStatementNode(IterStmtKind);

    match(WHILE);
    match(LPAREN);

    if(t != NULL)
    {
        t->child[0] = expression();
    }
    match(RPAREN);

    if(t != NULL)
    {
        t->child[1] = statement();
    }
    return t;
}


TreeNode * return_statement(void)
{
    TreeNode *t = newStatementNode(ReturnStmtKind);

    match(RETURN);

    if ((token.tokenType != SEMICOLON) && (t != NULL))
    {
        t->child[0] = expression();
    }
    match(SEMICOLON);
    return t;
}


TreeNode * expression(void)
{
    TreeNode *t;
    TreeNode *q = NULL;
    int flag = FALSE;

    if (token.tokenType == ID)
    {
        q = call();
        flag = TRUE;
    }

    if (flag == TRUE && token.tokenType == ASSIGN)
    {
        if ((q != NULL) && (q->nodekind == ExpKind) && (q->kind.expression == IdKind))
        {
            match(ASSIGN);

            t = newExpressionNode(AssignKind);
            if (t != NULL)
            {
                t->child[0] = q;
                t->child[1] = expression();
            }
        }
        else
        {
            syntaxError("attempt to assign to something not an lvalue\n");
            getToken(infile, outfile, reservedWords);
        }
    }
    else
    {
        t = simple_expression(q);
    }
    return t;
}



TreeNode * simple_expression(TreeNode *f)
{
    TreeNode *t;
    TreeNode *q;
    TokenType operator;

    q = add_expression(f);

    if (token.tokenType == LTHAN || token.tokenType == LTEQ || token.tokenType == GTHAN || 
        token.tokenType == GTEQ || token.tokenType == EQUAL || token.tokenType == NE)
    {
        operator = token.tokenType;

        match(token.tokenType);

        t = newExpressionNode(OpKind);
        if (t != NULL)
        {
            t->child[0] = q;
            t->child[1] = add_expression(NULL);
            t->attribute.operator = operator;
        }
    }
    else
    {
        t = q;
    }
    return t;
}


TreeNode * add_expression(TreeNode *f)
{
    TreeNode * t;
    TreeNode *q;

    t = term(f);

    if (t != NULL)
    {
        while (token.tokenType == PLUS || token.tokenType == MINUS)
        {
            q = newExpressionNode(OpKind);

            if (q != NULL)
            {
                q->child[0] = t;
                q->attribute.operator = token.tokenType;
                t = q;
                match(token.tokenType);
                t->child[1] = term(NULL);              
            }
        }
    }
    return t;
}



TreeNode * term(TreeNode *f)
{
    TreeNode * t;
    TreeNode *q;

    t = factor(f);

    if (t != NULL)
    {
        while (token.tokenType == MULT || token.tokenType == DIV)
        {
            q = newExpressionNode(OpKind);
            if (q != NULL)
            {
                q->child[0] = t;
                q->attribute.operator = token.tokenType;
                t = q;
                match(token.tokenType);
                t->child[1] = factor(NULL);               
            }
        }
    }
    return t;
}


TreeNode * factor(TreeNode *f)
{
    TreeNode *t;
    
    if (f != NULL)
    {
        return f;
    }

    switch (token.tokenType)
    {
        case LPAREN:
            match(LPAREN);
            t = expression();
            match(RPAREN);
            break;

        case ID:
            t = call();
            break;

        case NUM:
            t = newExpressionNode(ConstKind);
            if (t != NULL)
            {
                t->attribute.value = atoi(token.current_token);
                t->exp_type = Integer;
            }
            match(NUM);
            break;

        default: 
            syntaxError("unexpected token (factor) -> ");
            printTok(outfile, token.tokenType);
            getToken(infile, outfile, reservedWords);
            return Void;
    }
    return t;
}


TreeNode * call(void)
{
    TreeNode *t;
    char *name;

    if(token.tokenType == ID)
    {
        name = copyString(token.current_token);
    }
    match(ID);

    if (token.tokenType == LPAREN)
    {
        match(LPAREN);

        t = newStatementNode(CallKind);
        if (t != NULL)
        {
            t->attribute.name = name;
            t->child[0] = args();
        }
        match(RPAREN);
    }
    else if(token.tokenType == LBRACKET)
    {
        t = newExpressionNode(IdKind);

        if (t != NULL)
        {
            t->attribute.name = name;
            t->exp_type = Integer;
            match(LBRACKET);
            t->child[0] = expression();
            match(RBRACKET);
        }
    }
    else
    {
        t = newExpressionNode(IdKind);
        if (t != NULL)
        {
            t->attribute.name = name;
            t->exp_type = Integer;
        }
    }
    return t;
}


TreeNode * args(void)
{
    if (token.tokenType == RPAREN)
    {
        return NULL;
    }
    else
    {
        return args_list();
    }
}


TreeNode * args_list(void)
{
    TreeNode * t;
    TreeNode * p;

    t = expression();
    p = t;

    if (t != NULL)
    {
        while (token.tokenType == COMMA)
        {
            match(COMMA);

            TreeNode * q = expression();

            if (q != NULL)
            {
                if (t == NULL)
                {
                    t = p = q;
                }
                else 
                {
                    p->sibling = q;
                    p = q;
                }
            }
        }
    }
    return t;
}



TreeNode * parse(void)
{ 
    TreeNode * t;

    //get the first token
    getToken(infile, outfile, reservedWords);
    t = declaration_list();

    if (token.tokenType != ENDOFFILE)
    {
        syntaxError("Code ends before file\n");
    }
    return t;
}



static int indentno = 0;

/* macros to increase/decrease indentation */
#define INDENT indentno += 2
#define UNINDENT indentno -= 2



void printSpaces(void)
{
    for (int i = 0; i < indentno; i++)
    {
        fprintf(outfile, " ");
    }
}

char *typeName(ExpressionType exp_type)
{
    static char i[] = "int";
    static char v[] = "void";
    static char invalid[] = "<<invalid type>>";

    switch (exp_type)
    {
        case Integer:
            return i; 
            break;
        case Void:
            return v;
            break;
        default:
            return invalid;
    }
}




void printTree(TreeNode * tree)
{
    INDENT;
    while (tree != NULL)
    {
        printSpaces();

        if (tree->nodekind == StmtKind)
        {
            switch (tree->kind.statement)
            {
                case CompStmtKind:
                    fprintf(outfile, "Compound Statement:\n");
                    break;

                case SelectStmtKind:
                    if(tree->child[2] != NULL)
                    {
                        fprintf(outfile, "If (expression) statement else \n");
                    }
                    else
                    {
                        fprintf(outfile, "If (expression) statement\n");
                    }
                    break;

                case IterStmtKind:
                    fprintf(outfile, "While (expressioin) statement \n");
                    break;

                case ReturnStmtKind:
                    if(tree->child[0] == NULL)
                    {
                        fprintf(outfile, "Return Statement-without values\n");
                    }
                    else
                    {
                        fprintf(outfile, "Return Statement- with values\n");
                    }
                    break;

                case CallKind:
                    if(tree->child[0] != NULL)
                    {
                        fprintf(outfile, "Call, name: %s, with arguments\n",tree->attribute.name);
                    }
                    else
                    {
                        fprintf(outfile, "Call, name: %s, without arguments\n", tree->attribute.name);
                    }
                    break;

                default:
                    fprintf(outfile, "Unknown ExpressionNode kind\n");
                    break;
            }
        }
        else if (tree->nodekind == ExpKind)
        {
            switch (tree->kind.expression)
            {
                case VarDeclKind:
                    if(tree->isParam==TRUE)
                    {
                        fprintf(outfile, "Single Parameter, name: %s, type : %s\n", tree->attribute.name, typeName(tree->exp_type));
                    }
                    else
                    {
                        fprintf(outfile, "Varriable Declaration, name: %s, type : %s\n",tree->attribute.name, typeName(tree->exp_type));
                    }
                    break;

                case VarArrayDeclKind:
                    if(tree->isParam == TRUE)
                    {
                        fprintf(outfile, "Array Parameter, name: %s, type: %s\n", tree->attribute.name, typeName(tree->exp_type));
                    }
                    else
                        fprintf(outfile, "Array Var Declaration, name: %s, type: %s, size: %d\n", tree->attribute.name, typeName(tree->exp_type), tree->arraySize);
                    break;

                case FuncDeclKind:
                    fprintf(outfile, "Function Declaration, name: %s, type: %s\n", tree->attribute.name, typeName(tree->exp_type));
                    break;

                case AssignKind:
                    fprintf(outfile, "Assign: (destination) (source) \n");
                    break;

                case OpKind:
                    fprintf(outfile, "Op: \n");
                    //printTok(outfile, tree->attribute.operator);
                    break;
                case IdKind:
                    fprintf(outfile, "ID: %s\n",tree->attribute.name);
                    break;

                case ConstKind:
                    fprintf(outfile, "Constant: %d\n",tree->attribute.value);
                    break;

                default:
                    fprintf(outfile, "Unknown ExpressionNode kind\n");
                    break;
            }
        }
        else
        {
            fprintf(outfile, "Unknown node kind\n");
        } 
            
        for (int i = 0; i < MAXCHILDREN; i++)
        {
            printTree(tree->child[i]);
        }
        tree = tree->sibling;
    }
    UNINDENT;
}

