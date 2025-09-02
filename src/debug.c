#include <stdio.h>
#include "debug.h"
#include "parser.h"

void debug_print_ast(ASTNode* root, int indent) {
    for (int i = 0; i < indent; ++i) printf("  ");

    switch (root->type) {
        case AST_NODE_PROGRAM: {
            ASTNodeBlock* block = (ASTNodeBlock*)root;
            printf("Program:\n");
            for (int i = 0; i < block->count; ++i) {
                debug_print_ast(block->statements[i], indent + 1);
            }
        } break;
        case AST_NODE_BLOCK: {
            ASTNodeBlock* block = (ASTNodeBlock*)root;
            printf("Block:\n");
            for (int i = 0; i < block->count; ++i) {
                debug_print_ast(block->statements[i], indent + 1);
            }
        } break;
        case AST_NODE_IMPORT: {
            ASTNodeImport* import = (ASTNodeImport*)root;
            printf("Import: %s", import->path->data);
            if (import->name != NULL) {
                printf(" as %s\n", import->name->data);
            }
            else {
                printf("\n");
            }
        } break;
        case AST_NODE_FUNC_DECL: {
            ASTNodeFuncDecl* func_decl = (ASTNodeFuncDecl*)root;
            printf("FuncDecl: %s %d\n", func_decl->name->data, func_decl->param_count);
            debug_print_ast(func_decl->body, indent + 1);
        } break;
        case AST_NODE_VAR_DECL: {
            ASTNodeVarDecl* var_decl = (ASTNodeVarDecl*)root;
            printf("VarDecl: %s\n", var_decl->name->data);
            if (var_decl->initializer != NULL) {
                debug_print_ast(var_decl->initializer, indent + 1);
            }
        } break;
        case AST_NODE_EXPR_STMT: {
            ASTNodeExprStmt* expr_stmt = (ASTNodeExprStmt*)root;
            printf("ExprStmt:\n");
            debug_print_ast(expr_stmt->expression, indent + 1);
        } break;
        case AST_NODE_TERNARY:
        case AST_NODE_IF_STMT: {
            ASTNodeIfStmt* if_stmt = (ASTNodeIfStmt*)root;
            printf("If:\n");
            debug_print_ast(if_stmt->condition, indent + 1);
            for (int i = 0; i < indent; ++i) printf("  ");
            printf("Then:\n");
            debug_print_ast(if_stmt->then_branch, indent + 1);
            if (if_stmt->else_branch != NULL) {
                for (int i = 0; i < indent; ++i) printf("  ");
                printf("Else:\n");
                debug_print_ast(if_stmt->else_branch, indent + 1);
            }
        } break;
        case AST_NODE_WHILE_STMT: {
            ASTNodeWhileStmt* while_stmt = (ASTNodeWhileStmt*)root;
            printf("While:\n");
            debug_print_ast(while_stmt->condition, indent + 1);
            if (while_stmt->body != NULL) {
                for (int i = 0; i < indent; ++i) printf("  ");
                printf("Then:\n");
                debug_print_ast(while_stmt->body, indent + 1);
            }
        } break;
        case AST_NODE_FOR_STMT: {
            ASTNodeForStmt* for_stmt = (ASTNodeForStmt*)root;
            printf("For:\n");
            if (for_stmt->initializer != NULL) {
                debug_print_ast(for_stmt->initializer, indent + 1);
            }
            if (for_stmt->condition != NULL) {
                debug_print_ast(for_stmt->condition, indent + 1);
            }
            if (for_stmt->increment != NULL) {
                debug_print_ast(for_stmt->increment, indent + 1);
            }
            if (for_stmt->body != NULL) {
                for (int i = 0; i < indent; ++i) printf("  ");
                printf("Then:\n");
                debug_print_ast(for_stmt->body, indent + 1);
            }
        } break;
        case AST_NODE_RETURN_STMT: {
            ASTNodeExprStmt* return_stmt = (ASTNodeExprStmt*)root;
            printf("Return:");
            if (return_stmt->expression == NULL) {
                printf(" null\n");
            }
            else {
                putchar('\n');
                debug_print_ast(return_stmt->expression, indent + 1);
            }
        } break;
        case AST_NODE_BREAK: {
            printf("Break\n");
        } break;
        case AST_NODE_CONTINUE: {
            printf("Continue\n");
        } break;
        case AST_NODE_ASSIGNMENT: {
            ASTNodeAssignment* assignment = (ASTNodeAssignment*)root;
            printf("Assignment: %s\n", token_as_cstr(assignment->op));
            debug_print_ast(assignment->target, indent + 1);
            debug_print_ast(assignment->value, indent + 1);
        } break;
        case AST_NODE_LOGICAL: {
            ASTNodeBinary* binary = (ASTNodeBinary*)root;
            printf("Logical: %s\n", token_as_cstr(binary->op));
            debug_print_ast(binary->left, indent + 1);
            debug_print_ast(binary->right, indent + 1);
        } break;
        case AST_NODE_BINARY: {
            ASTNodeBinary* binary = (ASTNodeBinary*)root;
            printf("Binary: %s\n", token_as_cstr(binary->op));
            debug_print_ast(binary->left, indent + 1);
            debug_print_ast(binary->right, indent + 1);
        } break;
        case AST_NODE_UNARY: {
            ASTNodeUnary* unary = (ASTNodeUnary*)root;
            printf("Unary: %s\n", token_as_cstr(unary->op));
            debug_print_ast(unary->right, indent + 1);
        } break;
        case AST_NODE_CALL: {
            ASTNodeCall* call = (ASTNodeCall*)root;
            printf("Call: %d\n", call->count);
            for (int i = 0; i < indent; ++i) printf("  ");
            printf("Callee:\n");
            debug_print_ast(call->callee, indent + 1);
            for (int i = 0; i < indent; ++i) printf("  ");
            printf("Arguments:\n");
            for (int i = 0; i < call->count; ++i) {
                debug_print_ast(call->arguments[i], indent + 1);
            }
        } break;
        case AST_NODE_SUBSCRIPTION: {
            ASTNodeSubscription* subscription = (ASTNodeSubscription*)root;
            printf("Subscription:\n");
            for (int i = 0; i < indent; ++i) printf("  ");
            printf("Expression:\n");
            debug_print_ast(subscription->expression, indent + 1);
            for (int i = 0; i < indent; ++i) printf("  ");
            printf("Index:\n");
            debug_print_ast(subscription->index, indent + 1);
        } break;
        case AST_NODE_LITERAL: {
            ASTNodeLiteral* literal = (ASTNodeLiteral*)root;
            fputs("Literal: ", stdout);
            if (literal->value.type == VALUE_STRING) {
                putchar('"');
                print_value(literal->value);
                putchar('"');
            }
            else {
                print_value(literal->value);
            }
            fputs("\n", stdout);
        } break;
        case AST_NODE_VAR: {
            ASTNodeVar* var = (ASTNodeVar*)root;
            printf("Variable: %s\n", var->name->data);
        } break;
        case AST_NODE_LIST: {
            ASTNodeList* list = (ASTNodeList*)root;
            printf("List: %d\n", list->count);
            for (int i = 0; i < list->count; ++i) {
                debug_print_ast(list->expressions[i], indent + 1);
            }
        } break;
        default: {
            fprintf(stderr, "Unknown: ID=%d\n", root->type);
        } break;
    }
}
