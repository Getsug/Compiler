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

/*Global variable*/

/*holds current token data*/
TOKEN token;

/*function prototypes*/
void getToken(FILE *source, FILE *destination, Reserved *reservedWords);
char nextChar(FILE *source, FILE *destination);
void printToken(FILE *destination, FILE *source, TokenType tokType);
void getTok(FILE *source, FILE *destination, Reserved *reservedWords);
void printTok(FILE *source, FILE *destination, TokenType tokType);


int main(int argc, char *argv[])
{
    token.line_number = 0;
    token.character_loc = 0;
    token.line_len = 0;
    //token.token_index = 0;

    Reserved *reservedWords = (Reserved*)malloc(sizeof(Reserved) *RESERVE_SIZE);

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

    FILE *infile = fopen(argv[1], "r");
    FILE *outfile = fopen(argv[2], "w");

    /*while(fgets(token.line, BUF_SIZE - 1, infile))
    {
        printf("%s\n", token.line);
    }*/

    //getToken(infile, outfile, reservedWords);
    getTok(infile, outfile, reservedWords);

}


/*get the next character from line*/
char nextChar(FILE *source, FILE *destination)
{
    if(!(token.character_loc < token.line_len))
    {
        token.line_number++;

        if(fgets(token.line, BUF_SIZE - 1, source))
        {
            /*write to the outfile*/
            fprintf(destination, "%d:  %s", token.line_number, token.line);

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


void getTok(FILE *source, FILE *destination, Reserved *reservedWords)
{
    char c;
    int save;



    while(c != EOF)
    {
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



        printTok(source, destination, token.tokenType);
    }
}

void printTok(FILE *source, FILE *destination, TokenType tokType)
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
