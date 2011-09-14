#include "Node.h"

Node::Node()
    : parent(0), nextSibling(0), firstChild(0), type(Root), hash(0)
{}

Node::Node(Node *p, CXCursor c, const Location &l, uint h)
    : parent(p), nextSibling(0), firstChild(0), location(l), hash(h)
{
    const CXCursorKind kind = clang_getCursorKind(c);
    switch (kind) {
    case CXCursor_TypeRef:
        type = Reference; // This is more of a typeref than a forward declararion, rename?
        break;
    case CXCursor_StructDecl:
        type = clang_isCursorDefinition(c) ? Struct : Reference;
        break;
    case CXCursor_ClassDecl:
        type = clang_isCursorDefinition(c) ? Class : Reference;
        break;
    case CXCursor_MemberRefExpr:
    case CXCursor_CallExpr:
        type = Reference;
        break;
    case CXCursor_FieldDecl:
    case CXCursor_VarDecl:
    case CXCursor_ParmDecl:
        type = Variable;
        break;
    case CXCursor_MemberRef:
    case CXCursor_DeclRefExpr:
        type = Reference;
        break;
    case CXCursor_CXXMethod:
    case CXCursor_FunctionDecl:
    case CXCursor_Constructor:
    case CXCursor_Destructor:
    case CXCursor_FunctionTemplate:
    case CXCursor_ConversionFunction:
        type = clang_isCursorDefinition(c) ? MethodDefinition : MethodDeclaration;
        break;
    case CXCursor_Namespace:
        type = Namespace;
        break;
    case CXCursor_EnumDecl:
        type = Enum;
        break;
    case CXCursor_EnumConstantDecl:
        type = EnumValue;
        break;
    default:
        qDebug() << c << l << kindToString(clang_getCursorKind(c));
        Q_ASSERT(0 && "Can't find type for this cursor");
        break;
    }
    if (parent) {
        nextSibling = parent->firstChild;
        parent->firstChild = this;
        if (type == Reference && parent->type != Root) {
            // qDebug() << "doing parent reference" << c;
            symbolName = parent->symbolName;
        }
    }
    if (symbolName.isEmpty())
        symbolName = eatString(clang_getCursorDisplayName(c));
}


Node::~Node()
{
    while (firstChild) {
        Node *n = firstChild;
        firstChild = firstChild->nextSibling;
        delete n;
    }
}

QByteArray Node::toString() const
{
    if (type == Root)
        return "Root";
    int indent = 0;
    for (Node *p=parent; p; p = p->parent) {
        indent += 2;
    }
    QByteArray buf(indent, ' ');
    buf += typeToName(type);
    buf += ' ';
    buf += symbolName;
    buf += " [";
    buf += location.path;
    buf += ':';
    buf += QByteArray::number(location.line);
    buf += ':';
    buf += QByteArray::number(location.column);
    buf += ']';
    return buf;
}

void Node::print() const
{
    printf("%s\n", toString().constData());
    Node *child = firstChild;
    while (child) {
        child->print();
        child = child->nextSibling;
    }
}

const char *Node::typeToName(Type type, bool abbrev)
{
    switch (type) {
    case Enum: return abbrev ? "e" : "Enum";
    case EnumValue: return abbrev ? "ev" : "EnumValue";
    case Root: return abbrev ? "r" : "Root";
    case MethodDeclaration: return abbrev ? "ml" : "MethodDeclaration";
    case MethodDefinition: return abbrev ? "md" : "MethodDefinition";
    case Class: return abbrev ? "c" : "Class";
    case Struct: return abbrev ? "s" : "Struct";
    case Reference: return abbrev ? "pr" : "Reference";
    case Namespace: return abbrev ? "n" : "Namespace";
    case Variable: return abbrev ? "vd" : "Variable";
    case None:
    case All:
        break;
    }
    Q_ASSERT(0 && "Invalid type");
    return "Invalid";
}

int Node::size() const
{
    int ret = sizeof(*this);
    ret += symbolName.capacity() + 1;
    ret += location.path.capacity() + 1;
    for (Node *n=firstChild; n; n = n->nextSibling) {
        ret += n->size();
    }
    return ret;
}

Node *Node::methodDeclaration() const
{
    switch (type) {
    case Reference:
        Q_ASSERT(parent && parent->type == MethodDefinition);
        return parent->methodDeclaration();
    case MethodDeclaration:
        return const_cast<Node*>(this);
    case MethodDefinition:
        Q_ASSERT(parent);
        for (Node *n = parent->firstChild; n; n = n->nextSibling) {
            if (n->type == MethodDeclaration && n->symbolName == symbolName)
                return n;

        }
        return 0;
    default:
        break;
    }
    Q_ASSERT(0 && "This doesn't make any sense");
    return 0;
}

Node *Node::methodDefinition() const
{
    switch (type) {
    case Reference:
        Q_ASSERT(parent && parent->type == MethodDefinition);
        return parent;
    case MethodDefinition:
        return const_cast<Node*>(this);
    case MethodDeclaration:
        Q_ASSERT(parent);
        for (Node *n = parent->firstChild; n; n = n->nextSibling) {
            if (n->type == MethodDefinition && n->symbolName == symbolName)
                return n;
        }
        return 0;
    default:
        break;
    }
    Q_ASSERT(0 && "This doesn't make any sense");
    return 0;
}

void Node::add(Node *child)
{
#warning TODO, add inserted in debug mode
}

void Node::remove(Node *child)
{
#warning TODO
}
