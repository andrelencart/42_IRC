
NAME	=	ircserv

CXX		=	c++
CXXFLAGS=	-std=c++98 -Wall -Wextra -Werror -g

# -->┊( DIRECTORIES )
SRC_DIR	=	src
INC_DIR	=	includes
OBJ_DIR	=	objs

# -->┊( SOURCES AND OBJS )
SERVER_SRCS	=	Server.cpp \
				ClientIO.cpp \
				CommandProcessor.cpp \
				Registration.cpp \
				Join.cpp \
				ChannelCommands.cpp \
				Mode.cpp \
				Message.cpp \
				Lookup.cpp \
				Replies.cpp \
				Cleanup.cpp

CLIENT_SRCS	=	Client.cpp
CHANNEL_SRCS	=	Channel.cpp

SRCS	=	main.cpp \
			$(addprefix Server/, $(SERVER_SRCS)) \
			$(addprefix Client/, $(CLIENT_SRCS)) \
			$(addprefix Channel/, $(CHANNEL_SRCS))

OBJS	=	$(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))

# -->┊( RULES )
all: $(NAME)

$(NAME): $(OBJS)
	$(M_COMP)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	$(M_DONE)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)/Server $(OBJ_DIR)/Client $(OBJ_DIR)/Channel

clean:
	$(M_REMOBJS)
	@rm -rf $(OBJ_DIR)
	$(M_DONE)

fclean: clean
	$(M_REM)
	@rm -f $(NAME)
	$(M_DONE)

re: fclean all

.PHONY: all clean fclean re

# -->┊( COSMETICS )
DEF		=	\e[0;39m
BLK		=	\e[0;30m
BLU		=	\e[0;34m
GRN		=	\e[0;32m
BGRN	=	\e[1;32m
BWHT	=	\e[1;37m
WHTB	=	\e[47m

M_COMP		= @echo "$(BLK)-->┊$(GRN)  Compiling: $(DEF)$(BLK)$(WHTB) $(NAME) $(BLK)$(DEF)"
M_REM		= @echo "$(BLK)-->┊$(BLU)  Removing:  $(DEF)$(BLK)$(WHTB) $(NAME) $(BLK)$(DEF)"
M_REMOBJS	= @echo "$(BLK)-->┊$(BLU)  Removing:  $(DEF)$(BLK)$(WHTB) $(NAME)/objs $(BLK)$(DEF)"
M_DONE		= @echo "$(BLK)-->┊$(BGRN)  DONE!!$(DEF)"
