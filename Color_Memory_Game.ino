namespace Delays {
    enum : uint16_t {
        LONG = 1000,
        MEDIUM_LONG = 750,
        MEDIUM = 500,
        SHORT = 250,
    };
};

namespace Colors {
    enum ColorType : uint8_t {
        RED,
        BLUE,
        YELLOW,
        GREEN
    };

    constexpr Colors::ColorType colors[] = {
        Colors::RED,
        Colors::BLUE,
        Colors::YELLOW,
        Colors::GREEN
    };
    constexpr uint8_t num_colors = sizeof(colors) / sizeof(colors[0]);
};

namespace Leds {
    // The value of the enum variant corresponds to the pin.
    enum PinType : uint8_t {
        RED = 6,
        BLUE = 7,
        YELLOW = 8,
        GREEN = 9
    };

    void reset() {
        digitalWrite(Leds::RED, LOW);
        digitalWrite(Leds::BLUE, LOW);
        digitalWrite(Leds::YELLOW, LOW);
        digitalWrite(Leds::GREEN, LOW);
    }

    void all_on() {
        digitalWrite(Leds::RED, HIGH);
        digitalWrite(Leds::BLUE, HIGH);
        digitalWrite(Leds::YELLOW, HIGH);
        digitalWrite(Leds::GREEN, HIGH);
    }
}

namespace Buttons {
    /* The value of the enum variant corresponds to the pin. PinType 0 (RX) and
    pin 1 (TX) should not be connected to anything, because it interferes with
    uploading code. */
    enum PinType : uint8_t {
        RED = 2,
        BLUE = 3,
        YELLOW = 4,
        GREEN = 5,
    };

    constexpr Buttons::PinType buttons[] = {
        Buttons::RED,
        Buttons::BLUE,
        Buttons::YELLOW,
        Buttons::GREEN
    };
    constexpr uint8_t num_buttons = sizeof(buttons) / sizeof(buttons[0]);
}


Leds::PinType led_pin_of(Colors::ColorType color) {
    switch (color) {
        case Colors::RED: return Leds::RED;
        case Colors::BLUE: return Leds::BLUE;
        case Colors::YELLOW: return Leds::YELLOW;
        case Colors::GREEN: return Leds::GREEN;
        /* This line of code should never run, but it's necessary for the
    function to return a value for every input.  */
        default: return (Leds::PinType) 0;
    }
}

Leds::PinType button_to_led(Buttons::PinType pin) {
    switch (pin) {
        case Buttons::RED: return Leds::RED;  
        case Buttons::BLUE: return Leds::BLUE;  
        case Buttons::YELLOW: return Leds::YELLOW;  
        case Buttons::GREEN: return Leds::GREEN;
        /* This line of code should never run, but it's necessary for the
        function to return a value for every input.  */
        default: return (Leds::PinType) 0;  
    }
}

Colors::ColorType button_to_color(Buttons::PinType pin) {
    switch (pin) {
        case Buttons::RED: return Colors::RED;
        case Buttons::BLUE: return Colors::BLUE;
        case Buttons::YELLOW: return Colors::YELLOW;
        case Buttons::GREEN: return Colors::GREEN;
    }
}

bool button_is_pressed(Buttons::PinType pin) {
    return digitalRead(pin) == LOW;
}

Colors::ColorType get_button_input() {
    Buttons::PinType button_pin = (Buttons::PinType) 0;
    Leds::PinType led_pin;
    while (button_pin == 0) {
        /* Check to see if any of the buttons are being pressed */
        for (uint8_t i = 0; i < Buttons::num_buttons; i++) {
            if (button_is_pressed(Buttons::buttons[i])) {
                button_pin = Buttons::buttons[i];
                led_pin = button_to_led(button_pin);
                digitalWrite(led_pin, HIGH);
                break;
            }
        }
    }
    /* When a button is held, it should only count as one press. */
    while (button_is_pressed(button_pin)) {
        /* This delay is the prevent the Arduino from executing this polling
        loop more times than is necessary. It can be removed if necessary. */
        delay(Delays::SHORT);
    }
    digitalWrite(led_pin, LOW);
    return button_to_color(button_pin);
}

namespace Display {
    enum PinType : uint8_t {
        // Least significant bit
        BIT_ONE = 10,
        BIT_TWO = 11,
        BIT_THREE = 12,
        // Most significant bit
        BIT_FOUR = 13
    };

    constexpr Display::PinType pins[] = {
        Display::BIT_ONE,
        Display::BIT_TWO,
        Display::BIT_THREE,
        Display::BIT_FOUR
    };
    constexpr uint8_t num_pins = sizeof(pins) / sizeof(pins[0]);


    /* Gives an invalid value to the BCD decoder, resulting in a blank
    display */
    void reset() {
        digitalWrite(Display::BIT_ONE, HIGH);
        digitalWrite(Display::BIT_TWO, HIGH);
        digitalWrite(Display::BIT_THREE, HIGH);
        digitalWrite(Display::BIT_FOUR, HIGH);
    }

    void display_digit(uint8_t digit) {
        for (uint8_t i = 0; i < num_pins; i++) {
            uint8_t bit = bitRead(digit, i);
            if (bit == 1) {
                digitalWrite(pins[i], HIGH);
            } else {
                digitalWrite(pins[i], LOW);
            }
        }
    }

    void display_score(uint8_t score) {
        /* Can't take the logarithm of 0. */
        if (score == 0) display_digit(0);
        /* This method of counting the number of digits in a number works 
        because, for any 2 digit number n, 10 <= n < 100. Taking the logarithm
        base 10 of each expression shows that 1 <= log(n) < 2. This reasoning
        can be generalized for any number of digits. Normally, you would need to
        floor the value of the logarithm, but C++ automatically ignores decimal
        places when converting from a decimal to an integer. */
        const uint8_t num_digits = log10(score) + 1;
        for (uint8_t i = 0; i < num_digits; i++) {
            /* A number modulus 10 extracts the least significant digit from the
            number. To start with the most significant digit, divide 10^n where
            n is 1 less than the number of digits left. For example, 123 divided
            by 100 becomes 1.23 (truncated to 1), 1 % 10 is 1. For the second
            digit, 123 becomes 12.3 (12) % 10 = 2. So on and so forth. */
            uint8_t truncated_num = floor(score / pow(10, num_digits - i - 1));
            uint8_t digit = truncated_num % 10;
            display_digit(digit);
            delay(Delays::MEDIUM);
        }
    }
}

struct GameState {
    /* The longest a sequence can be is 256 colors, because that is the largest
    array which can be indexed using an unsigned 8-bit integer like `round`. */
    Colors::ColorType sequence[UINT8_MAX + 1];
    /* Represents the current length of the sequence, starting from 1. 0
    represents that no colors have been initialized. */
    uint8_t round;
    /* Represents if the player has made a mistake yet. Currently, the game
    doesn't have a win state. */ 
    bool has_lost;
};

/* This is the global struct used for representing the current game state. */
struct GameState game_state;

/* Initializes the values of the global game_state struct. This function MUST be
called in setup(), or else game_state will be using garbage values from memory.
*/
void initalize_game_state() {
    game_state.round = 0;
    game_state.has_lost = false;
}

void generate_next_color() {
    /* Generates a random color from RED until the last color (GREEN). The plus
    one is because the max is exclusive. */
    uint8_t index = random(Colors::RED, Colors::GREEN + 1);
    Colors::ColorType color = Colors::colors[index];
    game_state.sequence[game_state.round] = color;
    game_state.round++;
}

void display_sequence() {
    for (uint8_t i = 0; i < game_state.round; i++) {
        Leds::PinType pin = led_pin_of(game_state.sequence[i]);
        digitalWrite(pin, HIGH);
        // TODO: Remove magic numbers.
        delay(Delays::MEDIUM_LONG);
        digitalWrite(pin, LOW);
        delay(Delays::SHORT);
    }
}

void start_round() {
    generate_next_color();
    display_sequence();
    for (uint8_t i = 0; i < game_state.round; i++) {
        Colors::ColorType color = get_button_input();
        if (color != game_state.sequence[i]) {
            game_state.has_lost = true;
            break;
        }
    }
}

void display_start() {
    /* Light up in ascending order. */
    digitalWrite(Leds::RED, HIGH);
    delay(Delays::MEDIUM);
    digitalWrite(Leds::BLUE, HIGH);
    delay(Delays::MEDIUM);
    digitalWrite(Leds::YELLOW, HIGH);
    delay(Delays::MEDIUM);
    digitalWrite(Leds::GREEN, HIGH);
    delay(Delays::MEDIUM);
    /* Light up in alternating order. */
    Leds::reset();
    digitalWrite(Leds::RED, HIGH);
    digitalWrite(Leds::YELLOW, HIGH);
    delay(Delays::MEDIUM);
    Leds::reset();
    digitalWrite(Leds::BLUE, HIGH);
    digitalWrite(Leds::GREEN, HIGH);
    delay(Delays::MEDIUM);
}

void display_loss() {
    Leds::all_on();
    /* The current round is the one the player failed on, so their score is 1
    less than that. */
    Display::display_score(game_state.round - 1);
    delay(Delays::MEDIUM);
    Display::reset();
    delay(Delays::LONG * 2);
}

void setup() {
    /* Seeding the random number generation is necessary to have different
    results each time. Calling analogRead on an unconnected pin gives a
    relatively random seed. */
    randomSeed(analogRead(A0));
    initalize_game_state();
    /* Button pins should be configured as INPUT_PULLUP. A HIGH value indicates 
    the button isn't being pushed, and a LOW value indicates that it is being
    pushed. */
    pinMode(Buttons::RED, INPUT_PULLUP);
    pinMode(Buttons::BLUE, INPUT_PULLUP);
    pinMode(Buttons::YELLOW, INPUT_PULLUP);
    pinMode(Buttons::GREEN, INPUT_PULLUP);
    /* LED pins should be configured as OUTPUT. The circuit should be designed
    so that each LED only draws up to 20 mA of current. */
    pinMode(Leds::RED, OUTPUT);
    pinMode(Leds::BLUE, OUTPUT);
    pinMode(Leds::YELLOW, OUTPUT);
    pinMode(Leds::GREEN, OUTPUT);
    /* Display pins should be configured as outputs, since they are used to
    drive the seven segment display. */
    pinMode(Display::BIT_ONE, OUTPUT);
    pinMode(Display::BIT_TWO, OUTPUT);
    pinMode(Display::BIT_THREE, OUTPUT);
    pinMode(Display::BIT_FOUR, OUTPUT);
    /* Once all initialization is done, display the startup sequence. */
    display_start();
    Leds::reset();
    Display::reset();
    delay(Delays::LONG);
}

void loop() {
    if (!game_state.has_lost)  {
        start_round();
        /* Delay between the end of one round and the start of the next. */
        delay(Delays::LONG); 
    } else {
        display_loss();   
    }
}