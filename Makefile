# Les sources
SRC_DIR = src
HDR_DIR = headers
OBJ_DIR = obj

SOURCES = main.cpp $(SRC_DIR)/Server.cpp $(SRC_DIR)/Client.cpp
OBJECTS = $(OBJ_DIR)/main.o $(OBJ_DIR)/Server.o $(OBJ_DIR)/Client.o

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I$(HDR_DIR)

NAME = irc

all: $(OBJ_DIR) $(NAME)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.cpp $(HDR_DIR)/Server.hpp $(HDR_DIR)/Client.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HDR_DIR)/Server.hpp $(HDR_DIR)/Client.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAME): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@

clean:
	rm -f $(OBJ_DIR)/*.o

fclean: clean
	rm -rf $(NAME) $(OBJ_DIR)

re: fclean all

.PHONY: all clean fclean re

