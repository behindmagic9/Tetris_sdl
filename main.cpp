#include <SDL2/SDL.h>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <ctime>

#define WIDTH 400  // Increased width
#define HEIGHT 600  // Increased height
#define GRID_WIDTH 10  // Number of tiles wide in the playable grid
#define GRID_HEIGHT 20
#define TILE_SIZE 20

bool running = true;
bool gameOver = false;
SDL_Renderer* renderer;
SDL_Window* window;
SDL_Rect rect;

int score = 0;  // Score tracking

// Grid for the locked blocks
std::vector<std::vector<int>> grid(HEIGHT / TILE_SIZE, std::vector<int>(WIDTH / TILE_SIZE, 0));

// Structure for a Tetris block shape
struct shape {
    SDL_Color color;
    bool matrix[4][4];
    int x, y;  // Block position in grid units
    int size;  // Size of the block matrix
};

// Define the Tetris shapes (7 blocks)
shape blocks[7] = {
    {{255, 165, 0},  // L BLOCK
     {{0, 0, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, 3, 0, 3},
    {{255, 0, 0},  // Z BLOCK
     {{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, 3, 0, 3},
    {{224, 255, 255},  // I BLOCK
     {{1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, 3, 0, 4},
    {{0, 0, 255},  // J BLOCK
     {{1, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, 3, 0, 3},
    {{255, 255, 0},  // O BLOCK
     {{1, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, 3, 0, 2},
    {{0, 255, 0},  // S BLOCK
     {{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, 3, 0, 3},
    {{128, 0, 128},  // T BLOCK
     {{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, 3, 0, 3}
};

shape currentBlock;
shape nextBlock;  // New variable to store the next shape

// Function to generate a random block
shape generateBlock() {
    return blocks[rand() % 7];
}

// Rotates the block by transposing its matrix and reversing its columns
shape reverseCols(shape s) {
    shape tmp = s;
    for (int i = 0; i < s.size; i++) {
        for (int j = 0; j < s.size / 2; j++) {
            bool t = s.matrix[i][j];
            tmp.matrix[i][j] = s.matrix[i][s.size - j - 1];
            tmp.matrix[i][s.size - j - 1] = t;
        }
    }
    return tmp;
}

shape transpose(shape s) {
    shape tmp = s;
    for (int i = 0; i < s.size; i++) {
        for (int j = 0; j < s.size; j++) {
            tmp.matrix[i][j] = s.matrix[j][i];
        }
    }
    return tmp;
}

bool isValidMove(shape s, int dx, int dy) {
    for (int i = 0; i < s.size; i++) {
        for (int j = 0; j < s.size; j++) {
            if (s.matrix[i][j]) {
                int newX = s.x + i + dx;
                int newY = s.y + j + dy;
                if (newX < 0 || newX >= WIDTH / TILE_SIZE || newY >= HEIGHT / TILE_SIZE || (newY >= 0 && grid[newY][newX])) {
                    return false;
                }
            }
        }
    }
    return true;
}
// Rotate the block
void rotate() {
    shape temp = reverseCols(transpose(currentBlock));  // Rotate the block

    // Check if the rotated block goes out of the grid
    if (isValidMove(temp, 0, 0)) {
        currentBlock = temp;  // If valid, apply the rotation
    } else {
        // If the block would go out of bounds, adjust the position
        if (currentBlock.x + currentBlock.size > GRID_WIDTH) {
            currentBlock.x = GRID_WIDTH - currentBlock.size;  // Shift the block left to fit
        } else if (currentBlock.x < 0) {
            currentBlock.x = 0;  // Shift right if it's off the left edge
        }
    }
}

// Check if the current move is valid (no out-of-bounds or collision)

// Lock the block into the grid
void lockBlock() {
    for (int i = 0; i < currentBlock.size; i++) {
        for (int j = 0; j < currentBlock.size; j++) {
            if (currentBlock.matrix[i][j]) {
                grid[currentBlock.y + j][currentBlock.x + i] = 1;
            }
        }
    }
}

// Clear full lines and shift the rows down
void clearLines() {
    int clearedLines = 0;
    for (int y = HEIGHT / TILE_SIZE - 1; y >= 0; y--) {
        bool isFull = true;
        for (int x = 0; x < WIDTH / TILE_SIZE; x++) {
            if (!grid[y][x]) {
                isFull = false;
                break;
            }
        }
        if (isFull) {
            clearedLines++;
            for (int yy = y; yy > 0; yy--) {
                grid[yy] = grid[yy - 1];
            }
            grid[0] = std::vector<int>(WIDTH / TILE_SIZE, 0);
            y++;  // Check the current row again after shifting
        }
    }
    if (clearedLines > 0) {
        score += clearedLines * 100;
        std::cout << "Score: " << score << std::endl;
    }
}

// Draw the grid and the locked blocks
void drawGrid() {
    for (int y = 0; y < HEIGHT / TILE_SIZE; y++) {
        for (int x = 0; x < WIDTH / TILE_SIZE; x++) {
            if (grid[y][x]) {
                rect.x = x * TILE_SIZE;
                rect.y = y * TILE_SIZE;
                rect.w = TILE_SIZE;
                rect.h = TILE_SIZE;
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &rect);
                SDL_SetRenderDrawColor(renderer, 219, 219, 219, 255);
                SDL_RenderDrawRect(renderer, &rect);
            }
        }
    }
}

// Draw the current falling block
void draw(shape s) {
    for (int i = 0; i < s.size; i++) {
        for (int j = 0; j < s.size; j++) {
            if (s.matrix[i][j]) {
                rect.x = (s.x + i) * TILE_SIZE;
                rect.y = (s.y + j) * TILE_SIZE;
                rect.w = TILE_SIZE;
                rect.h = TILE_SIZE;
                SDL_SetRenderDrawColor(renderer, s.color.r, s.color.g, s.color.b, 255);
                SDL_RenderFillRect(renderer, &rect);
                SDL_SetRenderDrawColor(renderer, 219, 219, 219, 255);
                SDL_RenderDrawRect(renderer, &rect);
            }
        }
    }
}

// Draw the next block
void drawNextBlock() {
    // Render the next block in a smaller box aside of the main game area
    int startX = WIDTH + TILE_SIZE;  // Position aside the main grid
    int startY = TILE_SIZE * 2;

    for (int i = 0; i < nextBlock.size; i++) {
        for (int j = 0; j < nextBlock.size; j++) {
            if (nextBlock.matrix[i][j]) {
                rect.x = startX + (i * TILE_SIZE);
                rect.y = startY + (j * TILE_SIZE);
                rect.w = TILE_SIZE;
                rect.h = TILE_SIZE;
                SDL_SetRenderDrawColor(renderer, nextBlock.color.r, nextBlock.color.g, nextBlock.color.b, 255);
                SDL_RenderFillRect(renderer, &rect);
                SDL_SetRenderDrawColor(renderer, 219, 219, 219, 255);
                SDL_RenderDrawRect(renderer, &rect);
            }
        }
    }
}

void input() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            running = false;
        } else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_LEFT:
                    if (isValidMove(currentBlock, -1, 0)) currentBlock.x--;
                    break;
                case SDLK_RIGHT:
                    if (isValidMove(currentBlock, 1, 0)) currentBlock.x++;
                    break;
                case SDLK_DOWN:
                    if (isValidMove(currentBlock, 0, 1)) currentBlock.y++;
                    break;
                case SDLK_UP:
                    rotate();
                    break;
            }
        }
    }
}

// Update the game state every few milliseconds
void update() {
    static Uint32 lastUpdateTime = 0;
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastUpdateTime >= 500) {
        if (isValidMove(currentBlock, 0, 1)) {
            currentBlock.y++;
        } else {
            lockBlock();
            clearLines();
            currentBlock = nextBlock;  // Set the next block as the current one
            nextBlock = generateBlock();  // Generate a new next block
            if (!isValidMove(currentBlock, 0, 0)) gameOver = true;  // Check for game over
        }
        lastUpdateTime = currentTime;
    }
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    drawGrid();
    draw(currentBlock);
    drawNextBlock();  // Draw the next block on the side
        // Set color for the line (white in this case)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    // Draw the vertical line of separation between the grid and next block display
    SDL_RenderDrawLine(renderer, WIDTH, 0, WIDTH, HEIGHT);
    if (gameOver) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color for the message
        // Example rendering logic for the game over message (this is simplified)
        SDL_Rect gameOverRect = {WIDTH / 4, HEIGHT / 2 - 20, WIDTH / 2, 40}; // Adjust position and size as needed
        SDL_RenderFillRect(renderer, &gameOverRect); // Draw a filled rectangle for visibility

        // Print game over message (you can also use SDL_ttf for proper text rendering)
        std::cout << "Game Over! Final Score: " << score << std::endl;
    }
    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned>(time(0)));  // Seed the random generator
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH + 200, HEIGHT, SDL_WINDOW_SHOWN);  // Increase window width to accommodate next block
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    currentBlock = generateBlock();  // Initialize the first block
    nextBlock = generateBlock();  // Initialize the next block

    while (running && !gameOver) {
        input();
        update();
        render();
        SDL_Delay(16);  // Cap frame rate to ~60 FPS
    }

    std::cout << "Game Over! Final Score: " << score << std::endl;

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
