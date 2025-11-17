# --- Variáveis do Compilador ---

# O compilador que usaremos (gcc)
CC = gcc

# Flags de compilação:
# -Wall: Ativa quase todos os avisos (warnings)
# -g: Adiciona informações de debug (para usar com gdb)
# -I$(INCDIR): Diz ao compilador para procurar arquivos .h no diretório 'include'
CFLAGS = -Wall -g -I$(INCDIR)

# Flags do Linker (para bibliotecas externas, ex: -lm para matemática)
LDFLAGS =


# --- Variáveis de Diretório ---

# Nossos diretórios de origem
SRCDIR = src
INCDIR = include

# Diretórios de saída (onde os arquivos compilados irão)
OBJDIR = obj
BINDIR = bin


# --- Variáveis do Projeto ---

# Nome do executável final que queremos criar
TARGET = $(BINDIR)/MPI


# --- Descoberta Automática de Arquivos ---

# Encontra todos os arquivos .c no diretório 'src'
SOURCES = $(wildcard $(SRCDIR)/*.c)

# Mapeia os arquivos .c para arquivos .o no diretório 'obj'
# Ex: src/main.c se torna obj/main.o
# Ex: src/utils.c se torna obj/utils.o
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))


# --- Regras (Targets) ---

# A regra 'all' é a regra padrão. Ela apenas chama a regra $(TARGET).
# .PHONY significa que 'all' não é um arquivo real.
.PHONY: all
all: $(TARGET)

# Regra para criar o executável final:
# 1. Depende de todos os arquivos objeto ($(OBJECTS)).
# 2. Também depende que o diretório $(BINDIR) exista (usando | como pré-requisito de ordem).
$(TARGET): $(OBJECTS) | $(BINDIR)
	@echo "Ligando os arquivos objeto para criar o executável..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Executável '$(TARGET)' criado com sucesso!"

# Regra de padrão para compilar arquivos .c em .o:
# "Para criar qualquer arquivo em 'obj/' que termine em '.o',
#  procure um arquivo correspondente em 'src/' que termine em '.c'"
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@echo "Compilando $< -> $@"
	$(CC) $(CFLAGS) -c -o $@ $<

# Regras para criar os diretórios de saída se eles não existirem
$(BINDIR):
	@mkdir -p $@

$(OBJDIR):
	@mkdir -p $@


# --- Regras de Limpeza ---

# 'make clean' removerá todos os arquivos compilados.
.PHONY: clean
clean:
	@echo "Limpando arquivos compilados..."
	rm -rf $(OBJDIR) $(BINDIR)

# 'make run' irá compilar (se necessário) e executar o programa.
.PHONY: run
run: $(TARGET)
	@echo "Executando o programa..."
	./$(TARGET)