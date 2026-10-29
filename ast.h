#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    NODE_NUM,       // Числова константа
    NODE_VAR,       // Змінна (ідентифікатор)
    NODE_DECL,      // Оголошення: int x = ...
    NODE_ASSIGN,    // Присвоєння: x = ...
    NODE_BINOP,     // Бінарна операція: +, -, *, /, <, >, <=, >=, ==, !=
    NODE_IF,        // Умовний оператор: if (...) { ... } else { ... }
    NODE_WHILE,     // Цикл: while (...) { ... }
    NODE_PRINT,     // Вивід: print(...)
    NODE_SEQ        // Послідовність інструкцій (блок коду)
} NodeType;

typedef struct ASTNode {
    NodeType type;
    int num_val;              // Для NODE_NUM
    char var_name[64];        // Для NODE_VAR, NODE_DECL, NODE_ASSIGN
    char op[4];               // Для NODE_BINOP ("+", "-", "*", "/", "<", ">", "<=", ">=", "==", "!=")
    struct ASTNode *left;     // Лівий нащадок (або умова в IF/WHILE)
    struct ASTNode *right;    // Правий нащадок (або тіло THEN / тіло WHILE)
    struct ASTNode *else_branch; // Гілка ELSE для NODE_IF
} ASTNode;

typedef struct {
    char name[64];
    int value;
    int initialized;
} Symbol;

#define MAX_SYMBOLS 100
static Symbol symtab[MAX_SYMBOLS];
static int sym_count = 0;

static int get_var_value(const char *name) {
    for (int i = 0; i < sym_count; i++) {
        if (strcmp(symtab[i].name, name) == 0) return symtab[i].value;
    }
    fprintf(stderr, "[Семантична помилка]: Змінна '%s' не оголошена!\n", name);
    exit(1);
}

static void set_var_value(const char *name, int val, int is_decl) {
    for (int i = 0; i < sym_count; i++) {
        if (strcmp(symtab[i].name, name) == 0) {
            symtab[i].value = val;
            return;
        }
    }
    if (!is_decl) {
        fprintf(stderr, "[Семантична помилка]: Присвоєння неоголошеній змінній '%s'!\n", name);
        exit(1);
    }
    if (sym_count >= MAX_SYMBOLS) {
        fprintf(stderr, "[Помилка]: Забагато змінних (максимум %d)!\n", MAX_SYMBOLS);
        exit(1);
    }
    strncpy(symtab[sym_count].name, name, 63);
    symtab[sym_count].name[63] = '\0';
    symtab[sym_count].value = val;
    symtab[sym_count].initialized = 1;
    sym_count++;
}

//КОНСТРУКТОРИ ВУЗЛІВ AST
static ASTNode* create_node(NodeType type) {
    ASTNode *node = (ASTNode*)calloc(1, sizeof(ASTNode));
    node->type = type;
    return node;
}

static ASTNode* make_num(int val) {
    ASTNode *n = create_node(NODE_NUM);
    n->num_val = val;
    return n;
}

static ASTNode* make_var(const char *name) {
    ASTNode *n = create_node(NODE_VAR);
    strncpy(n->var_name, name, 63);
    return n;
}

static ASTNode* make_decl(const char *name, ASTNode *expr) {
    ASTNode *n = create_node(NODE_DECL);
    strncpy(n->var_name, name, 63);
    n->left = expr;
    return n;
}

static ASTNode* make_assign(const char *name, ASTNode *expr) {
    ASTNode *n = create_node(NODE_ASSIGN);
    strncpy(n->var_name, name, 63);
    n->left = expr;
    return n;
}

static ASTNode* make_binop(const char *op, ASTNode *l, ASTNode *r) {
    ASTNode *n = create_node(NODE_BINOP);
    strncpy(n->op, op, 3);
    n->left = l;
    n->right = r;
    return n;
}

static ASTNode* make_if(ASTNode *cond, ASTNode *then_b, ASTNode *else_b) {
    ASTNode *n = create_node(NODE_IF);
    n->left = cond;
    n->right = then_b;
    n->else_branch = else_b;
    return n;
}

static ASTNode* make_while(ASTNode *cond, ASTNode *body) {
    ASTNode *n = create_node(NODE_WHILE);
    n->left = cond;
    n->right = body;
    return n;
}

static ASTNode* make_print(ASTNode *expr) {
    ASTNode *n = create_node(NODE_PRINT);
    n->left = expr;
    return n;
}

static ASTNode* make_seq(ASTNode *first, ASTNode *second) {
    if (!first) return second;
    ASTNode *n = create_node(NODE_SEQ);
    n->left = first;
    n->right = second;
    return n;
}

//СЕМАНТИКА
static int eval_ast(ASTNode *node) {
    if (!node) return 0;
    switch (node->type) {
        case NODE_NUM: return node->num_val;
        case NODE_VAR: return get_var_value(node->var_name);
        case NODE_DECL: {
            int v = node->left ? eval_ast(node->left) : 0;
            set_var_value(node->var_name, v, 1);
            return v;
        }
        case NODE_ASSIGN: {
            int v = eval_ast(node->left);
            set_var_value(node->var_name, v, 0);
            return v;
        }
        case NODE_BINOP: {
            int l = eval_ast(node->left);
            int r = eval_ast(node->right);
            if (strcmp(node->op, "+") == 0) return l + r;
            if (strcmp(node->op, "-") == 0) return l - r;
            if (strcmp(node->op, "*") == 0) return l * r;
            if (strcmp(node->op, "/") == 0) {
                if (r == 0) { fprintf(stderr, "Помилка: Ділення на нуль!\n"); exit(1); }
                return l / r;
            }
            if (strcmp(node->op, "<") == 0) return l < r;
            if (strcmp(node->op, ">") == 0) return l > r;
            if (strcmp(node->op, "<=") == 0) return l <= r;
            if (strcmp(node->op, ">=") == 0) return l >= r;
            if (strcmp(node->op, "==") == 0) return l == r;
            if (strcmp(node->op, "!=") == 0) return l != r;
            return 0;
        }
        case NODE_IF:
            if (eval_ast(node->left)) return eval_ast(node->right);
            else if (node->else_branch) return eval_ast(node->else_branch);
            return 0;
        case NODE_WHILE:
            while (eval_ast(node->left)) {
                eval_ast(node->right);
            }
            return 0;
        case NODE_PRINT: {
            int v = eval_ast(node->left);
            printf("  [STDOUT] >> %d\n", v);
            return v;
        }
        case NODE_SEQ:
            eval_ast(node->left);
            return eval_ast(node->right);
    }
    return 0;
}

//ГЕНЕРАЦІЯ КОДУ НА ЦІЛЬОВІЙ МОВІ: JAVASCRIPT

static void js_indent(FILE *f, int level) {
    for (int i = 0; i < level; i++) fprintf(f, "    ");
}

static int is_compare(const char *op) {
    return strcmp(op, "<") == 0 || strcmp(op, ">") == 0 || strcmp(op, "<=") == 0 ||
           strcmp(op, ">=") == 0 || strcmp(op, "==") == 0 || strcmp(op, "!=") == 0;
}

static void gen_js_expr(ASTNode *node, FILE *f);

static void gen_js_compare(ASTNode *node, FILE *f) {
    const char *op = node->op;
    if (strcmp(op, "==") == 0) op = "===";
    if (strcmp(op, "!=") == 0) op = "!==";
    fprintf(f, "(");
    gen_js_expr(node->left, f);
    fprintf(f, " %s ", op);
    gen_js_expr(node->right, f);
    fprintf(f, ")");
}

static void gen_js_expr(ASTNode *node, FILE *f) {
    if (!node) return;
    switch (node->type) {
        case NODE_NUM: fprintf(f, "%d", node->num_val); break;
        case NODE_VAR: fprintf(f, "%s", node->var_name); break;
        case NODE_BINOP:
            if (strcmp(node->op, "/") == 0) {
                fprintf(f, "Math.trunc(");
                gen_js_expr(node->left, f);
                fprintf(f, " / ");
                gen_js_expr(node->right, f);
                fprintf(f, ")");
            } else if (is_compare(node->op)) {
                fprintf(f, "Number");
                gen_js_compare(node, f);
            } else {
                fprintf(f, "(");
                gen_js_expr(node->left, f);
                fprintf(f, " %s ", node->op);
                gen_js_expr(node->right, f);
                fprintf(f, ")");
            }
            break;
        default: break;
    }
}

static void gen_js_cond(ASTNode *cond, FILE *f) {
    if (cond && cond->type == NODE_BINOP && is_compare(cond->op)) {
        gen_js_compare(cond, f);
    } else if (cond && cond->type == NODE_BINOP && strcmp(cond->op, "/") != 0) {
        gen_js_expr(cond, f);
    } else {
        fprintf(f, "(");
        gen_js_expr(cond, f);
        fprintf(f, ")");
    }
}

static void gen_js_stmt(ASTNode *node, FILE *f, int level) {
    if (!node) return;
    switch (node->type) {
        case NODE_DECL:
            js_indent(f, level);
            fprintf(f, "let %s = ", node->var_name);
            if (node->left) gen_js_expr(node->left, f);
            else fprintf(f, "0");
            fprintf(f, ";\n");
            break;
        case NODE_ASSIGN:
            js_indent(f, level);
            fprintf(f, "%s = ", node->var_name);
            gen_js_expr(node->left, f);
            fprintf(f, ";\n");
            break;
        case NODE_IF:
            js_indent(f, level);
            fprintf(f, "if ");
            gen_js_cond(node->left, f);
            fprintf(f, " {\n");
            gen_js_stmt(node->right, f, level + 1);
            js_indent(f, level);
            fprintf(f, "}");
            if (node->else_branch) {
                fprintf(f, " else {\n");
                gen_js_stmt(node->else_branch, f, level + 1);
                js_indent(f, level);
                fprintf(f, "}");
            }
            fprintf(f, "\n");
            break;
        case NODE_WHILE:
            js_indent(f, level);
            fprintf(f, "while ");
            gen_js_cond(node->left, f);
            fprintf(f, " {\n");
            gen_js_stmt(node->right, f, level + 1);
            js_indent(f, level);
            fprintf(f, "}\n");
            break;
        case NODE_PRINT:
            js_indent(f, level);
            fprintf(f, "console.log(\"  [STDOUT] >> \" + ");
            gen_js_expr(node->left, f);
            fprintf(f, ");\n");
            break;
        case NODE_SEQ: 
            gen_js_stmt(node->left, f, level);
            gen_js_stmt(node->right, f, level);
            break;
        default:
            js_indent(f, level);
            gen_js_expr(node, f);
            fprintf(f, ";\n");
            break;
    }
}

static void gen_js(ASTNode *root, FILE *f) {
    fprintf(f, "// Код згенеровано транслятором з мови C у JavaScript\n");
    fprintf(f, "// Запуск: node out.js\n\n");
    gen_js_stmt(root, f, 0);
}

//ВІЗУАЛІЗАЦІЯ AST У БРАУЗЕРІ ЗАСОБАМИ D3.JS
static void ast_to_json(ASTNode *node, FILE *f) {
    if (!node) return;
    fprintf(f, "{\n");
    switch (node->type) {
        case NODE_NUM: fprintf(f, "  \"name\": \"NUM (%d)\", \"type\": \"literal\"", node->num_val); break;
        case NODE_VAR: fprintf(f, "  \"name\": \"VAR (%s)\", \"type\": \"var\"", node->var_name); break;
        case NODE_DECL: fprintf(f, "  \"name\": \"DECL (int %s)\", \"type\": \"decl\"", node->var_name); break;
        case NODE_ASSIGN: fprintf(f, "  \"name\": \"ASSIGN (%s =)\", \"type\": \"assign\"", node->var_name); break;
        case NODE_BINOP: fprintf(f, "  \"name\": \"OP (%s)\", \"type\": \"op\"", node->op); break;
        case NODE_IF: fprintf(f, "  \"name\": \"%s\", \"type\": \"control\"", node->else_branch ? "IF-ELSE" : "IF"); break;
        case NODE_WHILE: fprintf(f, "  \"name\": \"WHILE\", \"type\": \"control\""); break;
        case NODE_PRINT: fprintf(f, "  \"name\": \"PRINT\", \"type\": \"io\""); break;
        case NODE_SEQ: fprintf(f, "  \"name\": \"SEQ (Блок)\", \"type\": \"seq\""); break;
    }

    if (node->left || node->right || node->else_branch) {
        fprintf(f, ",\n  \"children\": [\n");
        int first = 1;
        if (node->left) {
            ast_to_json(node->left, f);
            first = 0;
        }
        if (node->right) {
            if (!first) fprintf(f, ",\n");
            ast_to_json(node->right, f);
            first = 0;
        }
        if (node->else_branch) {
            if (!first) fprintf(f, ",\n");
            ast_to_json(node->else_branch, f);
        }
        fprintf(f, "\n  ]");
    }
    fprintf(f, "\n}");
}

static void generate_d3_html(ASTNode *root, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return;

    fprintf(f, "<!DOCTYPE html>\n<html lang=\"uk\">\n<head>\n<meta charset=\"UTF-8\">\n");
    fprintf(f, "<title>Візуалізація AST (D3.js)</title>\n");
    fprintf(f, "<script src=\"https://d3js.org/d3.v7.min.js\"></script>\n");
    fprintf(f, "<style>\n"
               "  body { font-family: 'Segoe UI', system-ui, sans-serif; margin: 0; background: #0f172a; color: #f8fafc; overflow: hidden; }\n"
               "  .node circle { stroke-width: 2.5px; cursor: pointer; transition: r 0.2s; }\n"
               "  .node circle:hover { r: 11px; }\n"
               "  .node text { font-size: 13px; font-weight: 600; fill: #f1f5f9; text-shadow: 0 2px 4px rgba(0,0,0,0.9); }\n"
               "  .link { fill: none; stroke: #475569; stroke-width: 2px; }\n"
               "</style>\n</head>\n<body>\n");
    fprintf(f, "<script>\nconst treeData = ");
    ast_to_json(root, f);
    fprintf(f, ";\n\n"
               "const width = window.innerWidth, height = window.innerHeight;\n"
               "const svg = d3.select('body').append('svg').attr('width', width).attr('height', height);\n"
               "const g = svg.append('g').attr('transform', `translate(${width / 2}, 90)`);\n"
               "svg.call(d3.zoom().scaleExtent([0.2, 3]).on('zoom', (e) => g.attr('transform', e.transform)));\n\n"
               "const root = d3.hierarchy(treeData);\n"
               "const treeLayout = d3.tree().nodeSize([150, 100]);\n"
               "treeLayout(root);\n\n"
               "const colorMap = {\n"
               "  seq: '#64748b', decl: '#8b5cf6', assign: '#ec4899',\n"
               "  op: '#f59e0b', control: '#10b981', io: '#06b6d4',\n"
               "  var: '#3b82f6', literal: '#eab308'\n"
               "};\n\n"
               "g.selectAll('.link')\n"
               "  .data(root.links())\n"
               "  .join('path')\n"
               "  .attr('class', 'link')\n"
               "  .attr('d', d3.linkVertical().x(d => d.x).y(d => d.y));\n\n"
               "const node = g.selectAll('.node')\n"
               "  .data(root.descendants())\n"
               "  .join('g')\n"
               "  .attr('class', 'node')\n"
               "  .attr('transform', d => `translate(${d.x},${d.y})`);\n\n"
               "node.append('circle')\n"
               "  .attr('r', 8)\n"
               "  .attr('fill', d => colorMap[d.data.type] || '#38bdf8')\n"
               "  .attr('stroke', '#0f172a');\n\n"
               "node.append('text')\n"
               "  .attr('dy', -14)\n"
               "  .attr('text-anchor', 'middle')\n"
               "  .text(d => d.data.name);\n"
               "</script>\n</body>\n</html>\n");
    fclose(f);
}

#endif
