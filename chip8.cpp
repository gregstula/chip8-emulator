#include "chip8.hpp"
#include <chrono>
#include <fstream>
#include <thread>

namespace chip8 {
using namespace std::chrono_literals;
void vm::load_rom(std::string path)
{ std::ifstream rom(path, std::ios::binary | std::ios::ate);
    auto size = rom.tellg();
    rom.seekg(0, std::ios::beg);
    rom.read(reinterpret_cast<char*>(memory.data() + 512), size);
    program_counter = 512;
}

void vm::fetch()
{
    current_op = instruction(memory[program_counter], memory[program_counter + 1]);
    program_counter += 2;
}

void vm::execute()
{
    auto&& [type, x, y, n, nn, nnn] = current_op;

    switch (type) {
    case 0x0:
        // clear screen
        if (nn == 0xE0) {

            screen.fill(0);
        }

        // return from subroutine
        else if (nn == 0xEE) {
            program_counter = stack.top();
            stack.pop();
        }
        break;

    // jump to nnn
    case 0x1:
        program_counter = nnn;
        break;

    // call subroutine at nnn
    case 0x2:
        stack.push(program_counter);
        program_counter = nnn;
        break;

    // skip instruction if v[x] is nn
    case 0x3:
        if (V[x] == nn) program_counter += 2;
        break;
    // skip instruction if v[x] is not nn
    case 0x4:
        if (V[x] != nn) program_counter += 2;
        break;
    // skip instruction if v[x] == v[y]
    case 0x5:
        if (V[x] == V[y]) program_counter += 2;
        break;

    // set the register vX to nn
    case 0x6:
        V[x] = nn;
        break;

    // add nn to register v[x]
    case 0x7:
        V[x] += nn;
        break;

    // logical operations
    case 0x8:
        switch (n) {
        // set v[x] to v[y]
        case 0x0:
            V[x] = V[y];
            break;
        // bitwise OR v[x] v[y]
        case 0x1:
            V[x] |= V[y];
            break;
        // bitwise AND v[x] v[y]
        case 0x2:
            V[x] &= V[y];
            break;
        // bitwise XOR v[x] v[y]
        case 0x3:
            V[x] ^= V[y];
            break;
        // add v[x] v[y] set v[F] to 1 if carry
        // and preserve the lower 8 bits of addition only
        case 0x4: {
            int result = V[x] + V[y];
            if (result > 255) {
                V[0xF] = 1;
                result = result & 0xFF;
            }
            V[x] = result;
            break;
        }
        // subtraction v[x] - v[y]. if v[x] > v[y] then v[f] is set to 1
        case 0x5:
            V[0xF] = V[x] > V[y];
            V[x] -= V[y];
            break;

        // shift right by 1
        case 0x6:
            V[0xF] = V[x] & 1;
            V[x] /= 2;
            break;

         // subtraction v[y] - v[x]. if v[y] > v[x] then v[f] is set to 1
        case 0x7:
            V[0xF] = V[x] < V[y];
            V[x] = V[y] - V[x];
            break;

        // shift left
        case 0xE:
            V[0xF] = V[x] & 0b10000000;
            V[x] *= 2;
            break;

        }// switch
        break;

    // skip next intruction if Vx != Vy
    case 0x9:
          if (V[x] != V[y]) program_counter += 2;
          break;

    // set i register;
    case 0xA:
        index_reg = nnn;
        break;

    // jump to nnn + v[0]
    case 0xB:
        program_counter = nnn + V[0];
        break;
        // draw a sprite N rows talls at coords v[x] and v[y]
        // with the sprite data located with the I register
        // the sprite should modulo around the display but clip
    case 0xD: { // variable scope
        auto x_coord = V[x];
        auto y_coord = V[y];
        V[0xF] = 0;

        /* Example Sprite letter E of height n = 7
                 bit 7 6 5 4 3 2 1 0
        -------+--------------------
        byte 1 |     0 1 1 1 1 1 0 0
        byte 2 |     0 1 0 0 0 0 0 0
        byte 3 |     0 1 0 0 0 0 0 0
        byte 4 |     0 1 1 1 1 1 0 0
        byte 5 |     0 1 0 0 0 0 0 0
        byte 6 |     0 1 0 0 0 0 0 0
        byte 7 |     0 1 1 1 1 1 0 0
        */
        for (int byte = 0; byte < n; byte++) {
            // stop drawing if y coord reaches the bottom
            if (y_coord >= SCREEN_HEIGHT) break;

            auto sprite = memory[index_reg + byte];
            // byte length
            for (int bit = 0; bit < 8; bit++) {
                // if the pixel is on in the sprite row xor it with the screen coords
                auto coords = ((x_coord + bit) + SCREEN_WIDTH * (y_coord + byte)) % 2048;
                auto d_pix = screen[coords];
                // each bit in the sprite is a pixel
                auto s_pix = sprite & (0b10000000 >> bit);
                if (s_pix) {
                    screen[coords] ^= 1;
                    // if was originally 1 set to 1
                    V[0xF] = d_pix;
                }
            }
        }
        break;
    } // variable scope
    default:
        break;
    }
}

void vm::tick()
{
    fetch();
    execute();
    std::this_thread::sleep_for(60ms);
}

} // namespace chip8
