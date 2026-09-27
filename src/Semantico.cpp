#include "Semantico.h"
#include "Constants.h"
#include "SymbolType.h"

#include <limits>
#include <stdexcept>
#include <string>

namespace GalsGenerated {

namespace {

int parseArraySize(const Token &token) {
    std::string digits = token.getLexeme();
    int base = 10;

    switch (token.getId()) {
    case t_LIT_INT_HEX:
        digits = digits.substr(2);
        base = 16;
        break;
    case t_LIT_INT_BIN:
        digits = digits.substr(2);
        base = 2;
        break;
    case t_LIT_INT_DEC:
        break;
    default:
        throw SemanticError("Tamanho de vetor deve ser um literal inteiro", token.getPosition());
    }

    try {
        std::size_t parsedCharacters = 0;
        const long long size = std::stoll(digits, &parsedCharacters, base);

        if (parsedCharacters != digits.size() || size <= 0 || size > std::numeric_limits<int>::max()) {
            throw SemanticError("Tamanho de vetor invalido", token.getPosition());
        }

        return static_cast<int>(size);
    } catch (const std::invalid_argument &) {
        throw SemanticError("Tamanho de vetor invalido", token.getPosition());
    } catch (const std::out_of_range &) {
        throw SemanticError("Tamanho de vetor fora do limite", token.getPosition());
    }
}

// "heltal", "vetor de heltal", ...
std::string describeType(SymbolType type, bool isVector) {
    return isVector ? std::string("vetor de ") + symbolTypeToString(type) : symbolTypeToString(type);
}

std::string describe(const ExprType &expr) { return describeType(expr.type, expr.isVector); }

SemanticTable::Operation binaryOperationFor(TokenId token) {
    switch (token) {
    case t_ARTM_OP_PLUS:
        return SemanticTable::SUM;
    case t_ARTM_OP_MINUS:
        return SemanticTable::SUB;
    case t_ARTM_OP_MUL:
        return SemanticTable::MUL;
    case t_ARTM_OP_DIV:
        return SemanticTable::DIV;
    case t_ARTM_OP_MOD:
        return SemanticTable::MOD;
    case t_REL_OP_GT:
    case t_REL_OP_LT:
    case t_REL_OP_GT_EQ:
    case t_REL_OP_LT_EQ:
        return SemanticTable::REL;
    case t_REL_OP_EQ:
    case t_REL_OP_NOT_EQ:
        return SemanticTable::EQ;
    case t_LOG_OP_AND:
    case t_LOG_OP_OR:
        return SemanticTable::LOG;
    default: // << >> bitoch biteller bitxor
        return SemanticTable::BIT;
    }
}

SemanticTable::UnaryOperation unaryOperationFor(TokenId token) {
    switch (token) {
    case t_LOG_OP_NOT:
        return SemanticTable::LOG_NOT;
    case t_BIT_OP_NOT:
        return SemanticTable::BIT_NOT;
    default:
        return SemanticTable::NEG;
    }
}

} // namespace

Semantico::Semantico() { scopeStack.push_back(0); }

// ---------------------------------------------------------------------------
// Escopos
// ---------------------------------------------------------------------------

int Semantico::currentScope() const { return scopeStack.back(); }

std::string Semantico::currentScopeName() const { return symbolTable.getScope(currentScope()).name; }

std::string Semantico::scopeName(int scope) const { return symbolTable.getScope(scope).name; }

void Semantico::openScope(const std::string &name, std::optional<std::size_t> function) {
    scopeStack.push_back(symbolTable.createScope(name, currentScope(), function));
}

// Blocos recebem o nome da funcao em que estao + contador (ex.: somar1, somar2, global1)
void Semantico::openBlockScope() {
    const std::string base = functionStack.empty() ? "global" : symbolTable.at(functionStack.back()).getId();
    openScope(base + std::to_string(++blockCounter[base]));
}

void Semantico::closeScope() {
    if (scopeStack.size() <= 1) throw SemanticError("Fechamento de escopo sem escopo aberto");

    if (symbolTable.getScope(currentScope()).function.has_value()) functionStack.pop_back();
    scopeStack.pop_back();
}

// ---------------------------------------------------------------------------
// Tabela de simbolos
// ---------------------------------------------------------------------------

// Insere na tabela garantindo a unicidade no escopo atual
std::size_t Semantico::declare(const PendingId &id, SymbolType type, SymbolModality modality,
                               std::optional<int> vectorSize) {
    if (symbolTable.findInScope(id.name, currentScope()).has_value()) {
        throw SemanticError("Identificador '" + id.name + "' ja foi declarado no escopo '" + currentScopeName() + "'",
                            id.position);
    }

    return symbolTable.declare(Symbol(id.name, type, modality, currentScope(), id.position, vectorSize));
}

// Procura do escopo atual ate o global, sem olhar escopos ja fechados
std::optional<std::size_t> Semantico::findVisible(const std::string &name) const {
    for (auto scope = scopeStack.rbegin(); scope != scopeStack.rend(); ++scope) {
        if (auto found = symbolTable.findInScope(name, *scope)) return found;
    }

    return std::nullopt;
}

// Busca variavel/vetor/parametro visivel; erro se nao declarado ou se for funcao
std::size_t Semantico::findVariableOrThrow(const Token &token) {
    const auto found = findVisible(token.getLexeme());
    if (!found.has_value()) {
        throw SemanticError("Identificador '" + token.getLexeme() + "' nao foi declarado no escopo '" +
                                currentScopeName() + "'",
                            token.getPosition());
    }

    if (symbolTable.at(*found).isFunction()) {
        throw SemanticError("'" + token.getLexeme() + "' e uma sub-rotina e so pode ser usada em uma chamada",
                            token.getPosition());
    }

    return *found;
}

void Semantico::warn(const std::string &message, int position) { warnings.push_back({message, position}); }

void Semantico::warnIfUninitialized(std::size_t symbol, int position) {
    const Symbol &s = symbolTable.at(symbol);
    if (s.isInitialized() || warnedUninitialized.count(symbol)) return;

    warnedUninitialized.insert(symbol);
    warn("'" + s.getId() + "' esta sendo usado sem ter sido inicializado", position);
}

// ---------------------------------------------------------------------------
// Tipos
// ---------------------------------------------------------------------------

ExprType Semantico::popType(int position) {
    if (typeStack.empty()) throw SemanticError("Erro interno: pilha de tipos vazia", position);

    ExprType top = typeStack.back();
    typeStack.pop_back();
    return top;
}

// Desempilha os valores de uma lista (a partir da marca feita pela acao #9)
std::vector<ExprType> Semantico::popValueList(int position) {
    if (valueListMarks.empty()) throw SemanticError("Erro interno: lista de valores sem marca", position);

    const std::size_t mark = valueListMarks.back();
    valueListMarks.pop_back();

    std::vector<ExprType> values(typeStack.begin() + static_cast<long>(mark), typeStack.end());
    typeStack.resize(mark);
    return values;
}

void Semantico::checkAssignment(SymbolType leftType, bool leftIsVector, const ExprType &right,
                                const std::string &target, int position) {
    if (right.type == SymbolType::VOID) {
        throw SemanticError("Sub-rotina sem retorno nao produz valor para " + target, position);
    }

    if (leftIsVector != right.isVector) {
        throw SemanticError("Tipos incompativeis em " + target + ": esperado " + describeType(leftType, leftIsVector) +
                                ", recebido " + describe(right),
                            position);
    }

    switch (SemanticTable::atribType(leftType, right.type)) {
    case SemanticTable::ERR:
        throw SemanticError("Tipos incompativeis em " + target + ": nao e possivel atribuir " + describe(right) +
                                " a " + describeType(leftType, leftIsVector),
                            position);
    case SemanticTable::WAR:
        warn("Atribuicao de " + std::string(symbolTypeToString(right.type)) + " para " +
                 symbolTypeToString(leftType) + " em " + target + " (possivel perda de precisao)",
             position);
        break;
    default:
        break;
    }
}

void Semantico::checkIndex(const ExprType &index, const std::string &vectorName, int position) {
    if (index.isVector || (index.type != SymbolType::INT && index.type != SymbolType::ANY)) {
        throw SemanticError("Indice do vetor '" + vectorName + "' deve ser heltal, recebido " + describe(index),
                            position);
    }
}

void Semantico::binaryOperation(int position) {
    const ExprType right = popType(position);
    if (operatorStack.empty()) throw SemanticError("Erro interno: pilha de operadores vazia", position);
    const PendingOperator op = operatorStack.back();
    operatorStack.pop_back();
    const ExprType left = popType(position);

    if (left.isVector || right.isVector) {
        const std::string name = left.isVector ? left.name : right.name;
        throw SemanticError("Vetor '" + name + "' precisa de indice para ser usado na operacao '" + op.lexeme + "'",
                            op.position);
    }

    if (left.type == SymbolType::VOID || right.type == SymbolType::VOID) {
        throw SemanticError("Sub-rotina sem retorno usada na operacao '" + op.lexeme + "'", op.position);
    }

    const int result = SemanticTable::resultType(left.type, right.type, binaryOperationFor(op.token));
    if (result == SemanticTable::ERR) {
        throw SemanticError("Operacao '" + op.lexeme + "' invalida entre " + describe(left) + " e " +
                                describe(right),
                            op.position);
    }

    typeStack.push_back({static_cast<SymbolType>(result)});
}

void Semantico::unaryOperation(int position) {
    const ExprType operand = popType(position);
    if (operatorStack.empty()) throw SemanticError("Erro interno: pilha de operadores vazia", position);
    const PendingOperator op = operatorStack.back();
    operatorStack.pop_back();

    if (operand.isVector) {
        throw SemanticError("Vetor '" + operand.name + "' precisa de indice para ser usado na operacao '" +
                                op.lexeme + "'",
                            op.position);
    }

    const int result = SemanticTable::unaryResultType(operand.type, unaryOperationFor(op.token));
    if (result == SemanticTable::ERR) {
        throw SemanticError("Operacao '" + op.lexeme + "' invalida para " + describe(operand), op.position);
    }

    typeStack.push_back({static_cast<SymbolType>(result)});
}

// ---------------------------------------------------------------------------
// Acoes semanticas
// ---------------------------------------------------------------------------

void Semantico::executeAction(int action, const Token *token) {
    if (token == nullptr) throw SemanticError("Acao semantica executada sem token");

    const int pos = token->getPosition();

    switch (action) {
    // ----- Declaracao de variaveis e vetores -----
    case 1: { // ID em declaracao de variavel
        pendingIdentifiers.push_back({token->getLexeme(), pos});
        break;
    }
    case 2: { // Tipo em declaracao de variavel: insere todos os ids na tabela
        const auto type = symbolTypeFromToken(*token);
        const auto modality = pendingVectorSize.has_value() ? SymbolModality::VECTOR : SymbolModality::VARIABLE;

        lastDeclared.clear();
        for (const auto &identifier : pendingIdentifiers) {
            lastDeclared.push_back(declare(identifier, type, modality, pendingVectorSize));
        }

        pendingIdentifiers.clear();
        pendingVectorSize.reset();
        break;
    }
    case 3: { // Tamanho de vetor
        pendingVectorSize = parseArraySize(*token);
        break;
    }

    // ----- Declaracao de sub-rotinas -----
    case 4: { // ID de funcao: insere no escopo atual e abre o escopo da funcao
        const std::size_t function = declare({token->getLexeme(), pos}, SymbolType::VOID, SymbolModality::FUNCTION);
        functionStack.push_back(function);
        openScope(token->getLexeme(), function);
        pendingVectorSize.reset();
        break;
    }
    case 5: { // ID de parametro
        pendingParameter = PendingId{token->getLexeme(), pos};
        pendingVectorSize.reset();
        break;
    }
    case 6: { // Tipo de parametro
        if (!pendingParameter.has_value() || functionStack.empty()) {
            throw SemanticError("Parametro sem identificador", pos);
        }

        const auto type = symbolTypeFromToken(*token);
        const std::size_t parameter = declare(*pendingParameter, type, SymbolModality::PARAMETER, pendingVectorSize);
        // obs.: buscar a funcao so depois do declare(), pois inserir na tabela pode realocar o vetor de simbolos
        Symbol &function = symbolTable.at(functionStack.back());

        symbolTable.at(parameter).markInitialized();
        symbolTable.at(parameter).setParameterPosition(static_cast<int>(function.getParameters().size()) + 1);
        function.addParameter({type, pendingVectorSize.has_value()});

        pendingParameter.reset();
        pendingVectorSize.reset();
        break;
    }
    case 7: { // Tipo de retorno da funcao
        if (functionStack.empty()) throw SemanticError("Funcao sem identificador", pos);

        Symbol &function = symbolTable.at(functionStack.back());
        function.setType(symbolTypeFromToken(*token));
        function.setVectorSize(pendingVectorSize);
        pendingVectorSize.reset();
        break;
    }
    case 8: { // Funcao sem tipo de retorno (procedimento)
        if (functionStack.empty()) throw SemanticError("Funcao sem identificador", pos);

        symbolTable.at(functionStack.back()).setType(SymbolType::VOID);
        pendingVectorSize.reset();
        break;
    }

    // ----- Listas de valores (inicializacao e atribuicao) -----
    case 9: { // Inicio de lista de valores
        valueListMarks.push_back(typeStack.size());
        break;
    }
    case 10: { // Fim da inicializacao na declaracao: variabel a, b heltal = 1, 2;
        const auto values = popValueList(pos);

        if (values.size() != lastDeclared.size()) {
            throw SemanticError("Declaracao com " + std::to_string(lastDeclared.size()) + " identificador(es) e " +
                                    std::to_string(values.size()) + " valor(es)",
                                pos);
        }

        for (std::size_t i = 0; i < values.size(); i++) {
            Symbol &symbol = symbolTable.at(lastDeclared[i]);
            checkAssignment(symbol.getType(), symbol.isVector(), values[i],
                            "inicializacao de '" + symbol.getId() + "'", pos);
            symbol.markInitialized();
        }

        lastDeclared.clear();
        break;
    }

    // ----- Escopos -----
    case 11: { // Fim de bloco / funcao / for
        closeScope();
        break;
    }
    case 14:   // Inicio de bloco { ... }
    case 15: { // Inicio do cabecalho do for
        openBlockScope();
        break;
    }

    // ----- Retorno -----
    case 12: { // ge <exp>;
        const ExprType value = popType(pos);
        if (functionStack.empty()) throw SemanticError("'ge' usado fora de uma sub-rotina", pos);

        const Symbol &function = symbolTable.at(functionStack.back());
        if (function.getType() == SymbolType::VOID) {
            throw SemanticError("Procedimento '" + function.getId() + "' nao tem tipo de retorno e nao pode retornar valor",
                                pos);
        }

        checkAssignment(function.getType(), function.isVector(), value, "retorno de '" + function.getId() + "'", pos);
        break;
    }
    case 13: { // ge;
        if (functionStack.empty()) throw SemanticError("'ge' usado fora de uma sub-rotina", pos);

        const Symbol &function = symbolTable.at(functionStack.back());
        if (function.getType() != SymbolType::VOID) {
            throw SemanticError("Funcao '" + function.getId() + "' deve retornar um valor do tipo " +
                                    describeType(function.getType(), function.isVector()),
                                pos);
        }
        break;
    }

    // ----- Condicoes -----
    case 16: { // Condicao de om / medan / gör-medan / för
        const ExprType condition = popType(pos);
        if (condition.isVector || (condition.type != SymbolType::BOOL && condition.type != SymbolType::ANY)) {
            throw SemanticError("Condicao deve ser do tipo intelagom, recebido " + describe(condition), pos);
        }
        break;
    }

    // ----- Atribuicao -----
    case 17: { // ID que recebe a atribuicao
        attrTargets.push_back({findVariableOrThrow(*token), false, pos});
        break;
    }
    case 18: { // Indice do vetor que recebe a atribuicao
        const ExprType index = popType(pos);
        AttrTarget &target = attrTargets.back();
        const Symbol &symbol = symbolTable.at(target.symbol);

        if (!symbol.isVector()) throw SemanticError("'" + symbol.getId() + "' nao e um vetor", target.position);
        checkIndex(index, symbol.getId(), pos);
        target.indexed = true;
        break;
    }
    case 19: { // Fim da atribuicao: confere tipos e marca como inicializado
        const auto values = popValueList(pos);

        if (values.size() != attrTargets.size()) {
            throw SemanticError("Atribuicao com " + std::to_string(attrTargets.size()) + " destino(s) e " +
                                    std::to_string(values.size()) + " valor(es)",
                                pos);
        }

        for (std::size_t i = 0; i < values.size(); i++) {
            Symbol &symbol = symbolTable.at(attrTargets[i].symbol);
            const bool leftIsVector = symbol.isVector() && !attrTargets[i].indexed;
            checkAssignment(symbol.getType(), leftIsVector, values[i], "atribuicao a '" + symbol.getId() + "'",
                            attrTargets[i].position);
            symbol.markInitialized();
        }

        attrTargets.clear();
        break;
    }

    // ----- Uso de identificadores em expressoes -----
    case 20: { // ID usado em expressao
        const std::size_t index = findVariableOrThrow(*token);
        Symbol &symbol = symbolTable.at(index);

        symbol.markUsed();
        warnIfUninitialized(index, pos);
        typeStack.push_back({symbol.getType(), symbol.isVector(), symbol.getId()});
        break;
    }
    case 21: { // Indice de vetor usado em expressao: v[<exp>]
        const ExprType index = popType(pos);
        if (typeStack.empty()) throw SemanticError("Erro interno: pilha de tipos vazia", pos);

        ExprType &vector = typeStack.back();
        if (!vector.isVector) throw SemanticError("'" + vector.name + "' nao e um vetor", pos);
        checkIndex(index, vector.name, pos);
        vector.isVector = false;
        break;
    }

    // ----- Chamada de sub-rotinas -----
    case 22: { // ID da sub-rotina chamada
        const auto found = findVisible(token->getLexeme());
        if (!found.has_value()) {
            throw SemanticError("Sub-rotina '" + token->getLexeme() + "' nao foi declarada", pos);
        }

        Symbol &function = symbolTable.at(*found);
        if (!function.isFunction()) {
            throw SemanticError("'" + token->getLexeme() + "' nao e uma sub-rotina", pos);
        }

        function.markUsed();
        callStack.push_back({*found, typeStack.size(), pos});
        break;
    }
    case 27: { // Fim da chamada: confere quantidade e tipo dos argumentos
        const CallFrame call = callStack.back();
        callStack.pop_back();

        std::vector<ExprType> arguments(typeStack.begin() + static_cast<long>(call.typeStackMark), typeStack.end());
        typeStack.resize(call.typeStackMark);

        const Symbol &function = symbolTable.at(call.function);
        const auto &parameters = function.getParameters();

        if (arguments.size() != parameters.size()) {
            throw SemanticError("A sub-rotina '" + function.getId() + "' esperava " +
                                    std::to_string(parameters.size()) + " parametro(s) e foram passados " +
                                    std::to_string(arguments.size()),
                                call.position);
        }

        for (std::size_t i = 0; i < arguments.size(); i++) {
            checkAssignment(parameters[i].type, parameters[i].isVector, arguments[i],
                            "parametro " + std::to_string(i + 1) + " de '" + function.getId() + "'", call.position);
        }

        lastCallResult = {function.getType(), function.isVector()};
        break;
    }
    case 25: { // Resultado da chamada usado em expressao
        typeStack.push_back(lastCallResult);
        break;
    }
    case 28: { // std_läsa(): valor lido se adapta ao destino
        lastCallResult = {SymbolType::ANY};
        break;
    }
    case 29: { // Argumento do std_skriva(<exp>)
        const ExprType value = popType(pos);
        if (value.type == SymbolType::VOID) {
            throw SemanticError("Sub-rotina sem retorno nao pode ser escrita com std_skriva", pos);
        }
        lastCallResult = {SymbolType::VOID};
        break;
    }

    // ----- Incremento / decremento -----
    case 23:   // ID++ / ID--
    case 24: { // ++ID / --ID
        const std::size_t index = findVariableOrThrow(*token);
        Symbol &symbol = symbolTable.at(index);

        if (symbol.isVector() || (symbol.getType() != SymbolType::INT && symbol.getType() != SymbolType::FLOAT)) {
            throw SemanticError("Incremento/decremento exige heltal ou flyttal, mas '" + symbol.getId() + "' e " +
                                    describeType(symbol.getType(), symbol.isVector()),
                                pos);
        }

        symbol.markUsed();
        warnIfUninitialized(index, pos);
        symbol.markInitialized();
        lastIncrDecrResult = {symbol.getType()};
        break;
    }
    case 26: { // Resultado do incremento usado em expressao
        typeStack.push_back(lastIncrDecrResult);
        break;
    }

    // ----- Expressoes -----
    case 30: { // Literal
        switch (token->getId()) {
        case t_LIT_INT_DEC:
        case t_LIT_INT_HEX:
        case t_LIT_INT_BIN:
            typeStack.push_back({SymbolType::INT});
            break;
        case t_LIT_FLOAT:
            typeStack.push_back({SymbolType::FLOAT});
            break;
        case t_LIT_CHAR:
            typeStack.push_back({SymbolType::CHAR});
            break;
        case t_LIT_STRING:
            typeStack.push_back({SymbolType::STRING});
            break;
        case t_KW_TRUE:
        case t_KW_FALSE:
            typeStack.push_back({SymbolType::BOOL});
            break;
        default:
            throw SemanticError("Literal desconhecido", pos);
        }
        break;
    }
    case 31: { // Operador
        operatorStack.push_back({token->getId(), token->getLexeme(), pos});
        break;
    }
    case 32: // ou
    case 33: // och
    case 34: // biteller
    case 35: // bitxor
    case 36: // bitoch
    case 37: // relacionais
    case 38: // vänster / höger
    case 39: // plus / minus
    case 40: // gånger / delat / rest
        binaryOperation(pos);
        break;
    case 41: // minus / inte / bitinte (unario)
        unaryOperation(pos);
        break;

    default:
        throw SemanticError("Acao sem tratamento: #" + std::to_string(action), pos);
    }
}

void Semantico::finish() {
    const auto &symbols = symbolTable.getSymbols();

    for (const auto &symbol : symbols) {
        if (symbol.isUsed()) continue;

        const char *what = symbol.isFunction()    ? "Sub-rotina"
                           : symbol.isParameter() ? "Parametro"
                           : symbol.isVector()    ? "Vetor"
                                                  : "Variavel";
        warn(std::string(what) + " '" + symbol.getId() + "' declarado(a) e nao usado(a)", symbol.getSourcePosition());
    }
}

const SymbolTable &Semantico::getSymbolTable() const { return symbolTable; }

const std::vector<SemanticWarning> &Semantico::getWarnings() const { return warnings; }

} // namespace GalsGenerated
