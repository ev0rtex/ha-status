#define VERSION "0.1.0"

// Debugging headers
// #include <GDBStub.h>
#include <loopTimer.h>

// Networking/FS headers
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ArduinoHA.h>

// Display related headers
#include <PxMatrix.h>
#include <Ticker.h>
Ticker display_ticker;

#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// Define pins for the LED matrix
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_D 12
#define P_E 0
#define P_OE 2

// Sizing for the matrix
#define MATRIX_WIDTH        64
#define MATRIX_HEIGHT       32
#define MATRIX_ROW_PATTERN  16 // Row pattern 1/16

// Other sizing information
#define ICON_SIZE     16
#define TEXT_WIDTH    5
#define TEXT_HEIGHT   7
#define TEXT_SEP      1

// This defines the 'on' time of the display in µs. The larger this number, the brighter the
// display. If too large the ESP will crash
uint8_t display_draw_time = 60; // 30-70 is usually fine

// Instance to use for working with the display
PxMATRIX display(
  MATRIX_WIDTH,
  MATRIX_HEIGHT,
  P_LAT,
  P_OE,
  P_A,
  P_B,
  P_C,
  P_D
);

// Some standard colors
uint16_t pxRED = display.color565(255, 0, 0);
uint16_t pxGREEN = display.color565(0, 255, 0);
uint16_t pxBLUE = display.color565(0, 0, 255);
uint16_t pxWHITE = display.color565(255, 255, 255);
uint16_t pxCYAN = display.color565(0, 255, 255);
uint16_t pxMAGENTA = display.color565(255, 0, 255);
uint16_t pxYELLOW = display.color565(255, 255, 0);
uint16_t pxBLACK = display.color565(0, 0, 0);
uint16_t pxHABLUE = display.color565(65, 189, 245);

uint16_t pxCOLORS[8] = {pxRED, pxGREEN, pxBLUE, pxWHITE, pxCYAN, pxMAGENTA, pxYELLOW, pxBLACK};

// ISR for display refresh
void display_updater() {
  display.display(display_draw_time);
}

void display_update_enable(bool enable)
{
  if (enable) {
    display_ticker.attach(0.004, display_updater);
  } else {
    display_ticker.detach();
  }
}

/**
 * @brief Scroll text across the display
 *
 * @param ypos The y position to display the text
 * @param scroll_delay The delay between each scroll step
 * @param text The text to scroll
 * @param color The color to use for the text
 */
void scroll_text(uint8_t ypos, unsigned long scroll_delay, char* text, uint16_t color) {
  static uint32_t previous_time = 0;
  static int16_t xpos = MATRIX_WIDTH;

  // Check if it's time for a screen update
  if (millis() - previous_time >= scroll_delay) {
    previous_time = millis();
    display.setTextColor(color);
    display.clearDisplay();
    display.setCursor(xpos, ypos);
    display.println(text);
    --xpos;
  }

  // Reset once we've scrolled off the screen
  if (xpos <= -(MATRIX_WIDTH + strlen(text) * TEXT_WIDTH)) {
    xpos = MATRIX_WIDTH;
  }
}

/**
 * @brief Helper to convert a char* string to lowercase
 *
 * This function modifies the string in place so be sure you make a copy of the string you intend
 * to modify first if you don't want to modify the original
 *
 * @param string The string to convert
 */
void str_tolower(char* string) {
  for (int i = 0; i < strlen(string); ++i) {
    string[i] = tolower(string[i]);
  }
}

/**
 * @brief Joins an array of strings with a separator
 *
 * @param output The output string
 * @param strings The array of strings to join
 * @param num_strings The number of strings in the array
 * @param separator The separator to use
 */
void str_join(char* output, char* strings[], int num_strings, const char* separator) {
  output[0] = '\0';  // Set the initial output to be an empty string
  for (int i = 0; i < num_strings; i++) {
    strcat(output, strings[i]);
    if (i < num_strings - 1) {
      strcat(output, separator);
    }
  }
}

/**
 * @brief Draws a border around the display
 *
 * @param color The color to use for the border
 * @param size The size (thickness) of the border
 */
void draw_border(uint16_t color, int size = 1) {
  for (int x = 0; x < MATRIX_WIDTH; ++x) {
    for (int y = 0; y < size; ++y) {
      display.drawPixel(x, y, color);
      display.drawPixel(x, MATRIX_HEIGHT - 1 - y, color);
    }
  }
  for (int y = size; y < MATRIX_HEIGHT - size; ++y) {
    for (int x = 0; x < size; ++x) {
      display.drawPixel(x, y, color);
      display.drawPixel(MATRIX_WIDTH - 1 - x, y, color);
    }
  }
}

/**
 * Icons
 *
 * These are defined as 16x16 pixel icons where each number is the index of a color defined in the
 * pxCOLORS array above. The enum below is for convenience when naming an icon to use. There's
 * probably a better way to do this but it's what came to mind when I was writing this.
 */
enum Icons {
  Smile,
  Zoom,
  Calendar,
  Clock,
  HomeAssistant,
};
uint8_t static icons[][ICON_SIZE * ICON_SIZE] = {
  {
    7, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7,
    7, 7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7,
    7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7,
    7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7,
    7, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 6,
    7, 6, 6, 7, 3, 3, 7, 7, 7, 7, 3, 3, 7, 7, 6, 6,
    7, 6, 6, 7, 3, 7, 7, 7, 6, 7, 3, 7, 7, 7, 6, 6,
    7, 6, 6, 6, 7, 7, 7, 6, 6, 6, 7, 7, 7, 6, 6, 6,
    7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 6, 6, 6, 7,
    7, 7, 6, 6, 6, 6, 6, 7, 7, 7, 7, 6, 6, 6, 6, 7,
    7, 7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7,
    7, 7, 7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  },
  {
    7, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 7,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 3, 2, 2,
    2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 2, 2,
    2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 2,
    2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 2,
    2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 2, 2,
    2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    7, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 7,
  },
  {
    7, 7, 7, 7, 3, 3, 7, 7, 7, 7, 7, 3, 3, 7, 7, 7,
    7, 7, 2, 2, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 2, 7,
    7, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    7, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    7, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    7, 2, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 2,
    7, 2, 7, 7, 7, 7, 3, 3, 7, 3, 3, 7, 3, 3, 7, 2,
    7, 2, 7, 7, 7, 7, 3, 3, 7, 3, 3, 7, 3, 3, 7, 2,
    7, 2, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 2,
    7, 2, 7, 3, 3, 7, 3, 3, 7, 3, 3, 7, 3, 3, 7, 2,
    7, 2, 7, 3, 3, 7, 3, 3, 7, 3, 3, 7, 3, 3, 7, 2,
    7, 2, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 2,
    7, 2, 7, 3, 3, 7, 3, 3, 7, 7, 7, 7, 7, 7, 7, 2,
    7, 2, 7, 3, 3, 7, 3, 3, 7, 7, 7, 7, 7, 7, 7, 2,
    7, 2, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 2,
    7, 7, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 7,
  },
  {
    7, 7, 7, 7, 7, 3, 3, 3, 3, 3, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 3, 3, 7, 7, 7, 7, 7, 3, 3, 7, 7, 7, 7,
    7, 7, 3, 7, 7, 7, 7, 3, 7, 7, 7, 7, 3, 7, 7, 7,
    7, 3, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 3, 7, 7,
    7, 3, 7, 7, 2, 7, 7, 7, 7, 7, 7, 7, 7, 3, 7, 7,
    3, 7, 7, 7, 7, 2, 7, 7, 7, 7, 7, 7, 7, 7, 3, 7,
    3, 7, 7, 7, 7, 7, 2, 7, 2, 7, 7, 7, 7, 7, 7, 7,
    3, 7, 3, 7, 7, 7, 7, 2, 7, 7, 7, 7, 7, 1, 7, 7,
    3, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 1, 1, 1, 7,
    3, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 1, 7, 1, 7, 1,
    7, 3, 7, 7, 7, 7, 7, 7, 7, 7, 7, 1, 7, 1, 7, 7,
    7, 3, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 1, 1, 1, 7,
    7, 7, 3, 7, 7, 7, 7, 3, 7, 7, 7, 7, 7, 1, 7, 1,
    7, 7, 7, 3, 3, 7, 7, 7, 7, 7, 7, 1, 7, 1, 7, 1,
    7, 7, 7, 7, 7, 3, 3, 3, 3, 3, 7, 7, 1, 1, 1, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 1, 7, 7,
  },
  {
    7, 7, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 7,
    7, 2, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2,
    7, 2, 2, 2, 2, 2, 2, 3, 3, 3, 2, 2, 2, 2, 2, 2,
    7, 2, 2, 2, 2, 2, 3, 3, 2, 3, 3, 2, 2, 2, 2, 2,
    7, 2, 2, 2, 2, 3, 3, 2, 3, 2, 3, 3, 3, 3, 2, 2,
    7, 2, 2, 2, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 2, 2,
    7, 2, 2, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 2, 2,
    7, 2, 3, 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 3, 2,
    7, 2, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 2,
    7, 2, 2, 3, 3, 2, 2, 3, 2, 3, 2, 2, 3, 3, 2, 2,
    7, 2, 2, 3, 3, 3, 3, 2, 2, 2, 3, 3, 3, 3, 2, 2,
    7, 2, 2, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 2, 2,
    7, 2, 2, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 2, 2,
    7, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    7, 7, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  },
};
const int NUM_ICONS = sizeof(icons) / sizeof(icons[0]);

// Draw an icon from the icon set at the given x, y offset
void draw_icon(int16_t x_off, int16_t y_off, uint8_t icon) {
  for (int y = 0; y < 16; ++y) {
    for (int x = 0; x < 16; ++x) {
      display.drawPixel(x_off + x, y_off + y, pxCOLORS[icons[icon][y * 16 + x]]);
    }
  }
}

// HA + MQTT variables
byte mac[6];
WiFiClient wifi_client;
HADevice device;
HAMqtt mqtt(wifi_client, device);
HASelect Select("STATUS");

/**
 * Scenes
 */
struct Scene {
  int16_t border_color;
  Icons icon;
  char name[8];
};
Scene scenes[] = {
  {pxGREEN  , Smile    , "Free"},
  {pxRED    , Zoom     , "Zoom"},
  {pxYELLOW , Calendar , "Busy"},
  {pxYELLOW , Clock    , "Work"},
};
const int NUM_SCENES = sizeof(scenes) / sizeof(Scene);

// Helper to find a scene by name
Scene* find_scene_by_name(String name);
Scene* find_scene_by_name(String name) {
  for (int i = 0; i < NUM_SCENES; ++i) {
    if (name.equalsIgnoreCase(scenes[i].name)) {
      return &scenes[i];
    }
  }
  return NULL;
}

/**
 * @brief Gets the names of all scenes
 * @param names An array of strings to store the names of all scenes
 */
void scene_names(char** names) {
  for (int i = 0; i < NUM_SCENES; ++i) {
    names[i] = new char[sizeof(Scene::name)];
    strncpy(names[i], scenes[i].name, sizeof(Scene::name));
  }
}

// The amount of time for a scene transition (fade-in/fade-out)
#define SCENE_TRANSITION_TIME 1000

/**
 * @brief Scene manager class
 *
 * This class handles the transition between scenes. It will fade out the current scene and fade in
 * the new scene. This is implemented in a non-blocking way so that it can be called from the main
 * loop without causing any issues.
 */
class SceneManager {
  public:
    enum SceneState {
      Idle,
      FadeIn,
      FadeOut,
    };

    SceneManager()
      : current_scene(NULL)
      , state(Idle)
      , transition_start(0)
      , brightness(255)
    {}

    /**
     * @brief Render a scene
     *
     * This renders a given scene to the display by calculating offsets and drawing the border, icon,
     * and text.
     *
     * @param scene The scene to render
     */
    void render(Scene* scene) {
      // Clear the display
      display.clearDisplay();
      if (scene == NULL) {
        return;
      }

      // Render the border
      draw_border(scene->border_color, 2);

      // Render the icon
      int16_t hpos = ceil((MATRIX_WIDTH - 2 - (ICON_SIZE + 2) - (strlen(scene->name) * TEXT_WIDTH) - ((strlen(scene->name) - 1) * TEXT_SEP)) / 2);
      int16_t vpos = ceil((MATRIX_HEIGHT - ICON_SIZE - 2) / 2);
      draw_icon(hpos, vpos, scene->icon);

      // Render the text
      display.setCursor(hpos + ICON_SIZE + 2, vpos + ceil((ICON_SIZE - TEXT_HEIGHT) / 2));
      display.setTextColor(pxWHITE);
      display.print(scene->name);
    }

    void show(Scene* scene) {
      // See if we need to show a new scene and if so, start the transition
      if (current_scene != scene) {
        current_scene = scene;
        state = FadeOut;
        transition_start = millis();
        select_update();
      }
    }

    /**
     * @brief Scene manager state machine loop
     *
     * This should be called from the main loop to update the scene manager. It will handle the
     * transition between scenes.
     */
    void loop() {
      switch (state) {
        case FadeOut:
          // Fade out the current scene
          brightness = map(millis() - transition_start, 0, SCENE_TRANSITION_TIME, 255, 0);
          if (brightness <= 0) {
            brightness = 0;
            state = FadeIn;
            render(current_scene);
            transition_start = millis();
          }
          display.setBrightness(brightness);
          break;
        case FadeIn:
          // Fade in the new scene
          brightness = map(millis() - transition_start, 0, SCENE_TRANSITION_TIME, 0, 255);
          if (brightness >= 255) {
            brightness = 255;
            state = Idle;
          }
          display.setBrightness(brightness);
          break;
        case Idle:
          // Do nothing
          break;
      }
    }

  private:
    Scene* current_scene;
    SceneState state;
    uint32_t transition_start;
    uint8_t brightness;

    void select_update() {
      // Update the select state with the current scene index
      uint8_t current_scene_index = 0;
      for (int i = 0; i < NUM_SCENES; ++i) {
        if (current_scene == &scenes[i]) {
          current_scene_index = i;
          break;
        }
      }
      Select.setState(current_scene_index);
    }
};
SceneManager scene_manager;

enum DisplayMode {
  Demo,
  Live,
};
DisplayMode display_mode = Demo;

// Duration to show each scene
#define SCENE_DEMO_TIME 2000

/**
 * Scene demo class
 */
class SceneDemo {
  private:
    uint32_t scene_start;
    uint8_t scene_index;
    char** scene_list;

  public:
    SceneDemo()
      : scene_index(0)
      , scene_start(0)
    {
      scene_list = new char*[NUM_SCENES];
      scene_names(scene_list);
      // For testing... lowercase all scene names to make sure find_scene_by_name is case-insensitive
      for (uint8_t i = 0; i < NUM_SCENES; ++i) {
        str_tolower(scene_list[i]);
      }
    }

    ~SceneDemo() {
      for (uint8_t i = 0; i < NUM_SCENES; ++i) {
        delete[] scene_list[i];
      }
      delete[] scene_list;
    }

    /**
     * @brief Run the scene demo
     */
    void loop() {
      if (millis() - scene_start > (SCENE_DEMO_TIME + SCENE_TRANSITION_TIME * 2)) {
        scene_start = millis();
        // show_scene(find_scene_by_name(scene_list[scene_index]));
        scene_manager.show(find_scene_by_name(scene_list[scene_index]));
        scene_index = (scene_index + 1) % NUM_SCENES;
      }
    }
};

/**
 * Dynamic configuration/setup
 */

#define CONFIG_FILE "/config.json"
#define CONFIG_FILE_SIZE 1024

// MQTT configuration
struct MqttConfig {
  char host[120]    = "mqtt.example.com";
  char port[6]      = "1883";
  char username[40] = "mqttuser";
  char password[40] = "mqttpass";
} mqtt_config;

// Global config struct
struct Config {
  MqttConfig* mqtt = &mqtt_config;
} config;

// Custom WiFiManager parameters
WiFiManagerParameter mqtt_host("mqtt_host", "MQTT Host", mqtt_config.host, 120);
WiFiManagerParameter mqtt_port("mqtt_port", "MQTT Port", mqtt_config.port, 6);
WiFiManagerParameter mqtt_username("mqtt_username", "MQTT Username", mqtt_config.username, 40);
WiFiManagerParameter mqtt_password("mqtt_password", "MQTT Password", mqtt_config.password, 40);

/**
 * @brief Loads configuration
 *
 * This function will load the configuration from the config file and update the custom parameters
 * with the values.
 */
void load_config() {
  // Open the config file
  File file = LittleFS.open(CONFIG_FILE, "r");
  if (!file) {
    Serial.println("Failed to open config file... using default config");
    return;
  }

  // Read the config file
  size_t size = file.size();
  std::unique_ptr<char[]> buf(new char[size]);
  file.readBytes(buf.get(), size);
  DynamicJsonDocument doc(max(static_cast<size_t>(CONFIG_FILE_SIZE), size));
  DeserializationError error = deserializeJson(doc, buf.get());
  if (error) {
    Serial.println("Failed to parse config file");
    return;
  }

  // Copy the config values
  strcpy(mqtt_config.host, doc["mqtt"]["host"] | mqtt_config.host);
  strcpy(mqtt_config.port, doc["mqtt"]["port"] | mqtt_config.port);
  strcpy(mqtt_config.username, doc["mqtt"]["username"] | mqtt_config.username);
  strcpy(mqtt_config.password, doc["mqtt"]["password"] | mqtt_config.password);

  // Update the custom parameters
  mqtt_host.setValue(mqtt_config.host, 120);
  mqtt_port.setValue(mqtt_config.port, 6);
  mqtt_username.setValue(mqtt_config.username, 40);
  mqtt_password.setValue(mqtt_config.password, 40);
  char maskedPassword[40] = {}; for (int i = 0; i < strlen(mqtt_config.password); ++i) maskedPassword[i] = '*';

  Serial.println("Loaded configuration:");
  Serial.printf("  MQTT Host: %s\r\n", mqtt_config.host);
  Serial.printf("  MQTT Port: %s\r\n", mqtt_config.port);
  Serial.printf("  MQTT Username: %s\r\n", mqtt_config.username);
  Serial.printf("  MQTT Password: %s\r\n", maskedPassword);
}

/**
 * @brief Saves configuration
 *
 * This function will save the current configuration to the config file after pulling the values
 * from the custom parameters.
 */
void save_config() {
  // Open the config file
  File file = LittleFS.open(CONFIG_FILE, "w");
  if (!file) {
    Serial.println("Failed to open config file for writing");
    return;
  }

  // Update the config values from the custom parameters
  strcpy(mqtt_config.host, mqtt_host.getValue());
  strcpy(mqtt_config.port, mqtt_port.getValue());
  strcpy(mqtt_config.username, mqtt_username.getValue());
  strcpy(mqtt_config.password, mqtt_password.getValue());

  // Write the config file
  DynamicJsonDocument doc(CONFIG_FILE_SIZE);
  JsonObject mqtt_json = doc.createNestedObject("mqtt");
  mqtt_json["host"] = mqtt_config.host;
  mqtt_json["port"] = mqtt_config.port;
  mqtt_json["username"] = mqtt_config.username;
  mqtt_json["password"] = mqtt_config.password;
  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write to config file");
  } else {
    Serial.println("Configuration saved successfully");
    // (Re)establish the MQTT connection
    mqtt.disconnect();
    mqtt.begin(
      mqtt_config.host,
      atoi(mqtt_config.port),
      mqtt_config.username,
      mqtt_config.password
    );
  }
}

/**
 * Setup and main loop
 */

#define WIFI_CONFIG_AP_NAME "StatusSign Setup AP"

WiFiManager wm;
SceneDemo scene_demo;

// Setup tasks
void setup() {
  Serial.begin(115200);
  // gdbstub_init();

  // Set up the display
  display.begin(MATRIX_ROW_PATTERN);
  display.clearDisplay();
  display.setBrightness(255 / 2);
  display_update_enable(true);

  // Home Assistant logo
  draw_icon((MATRIX_WIDTH - ICON_SIZE) / 2 - 1, 2, Icons::HomeAssistant);

  // Draw the device logo text
  int16_t x = 0, y = 0;
  uint16_t w = 0, h = 0;
  char logo_status[] = "status";

  display.setFont(&FreeSans9pt7b);
  display.getTextBounds(logo_status, 2, 2, &x, &y, &w, &h);
  display.setCursor((MATRIX_WIDTH - w) / 2 - 1, MATRIX_HEIGHT - 2 - 1);
  display.setTextColor(pxHABLUE);
  display.print(logo_status);

  // Reset the font and text color
  display.setFont();
  display.setTextColor(pxWHITE);

  // Load the configuration
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    return;
  }
  load_config();

  // Setup WiFi and the WiFiManager instance
  WiFi.mode(WIFI_STA);
  wm.setHostname("ha-status");
  wm.setTitle("Status Sign Configuration");
  std::vector<const char*> menu = {
    "wifi",
    "info",
    "param",
    "sep",
    "restart",
    "exit"
  };
  wm.setMenu(menu);
  wm.setClass("invert");
  wm.setParamsPage(true);

  // Add custom parameters
  wm.addParameter(&mqtt_host);
  wm.addParameter(&mqtt_port);
  wm.addParameter(&mqtt_username);
  wm.addParameter(&mqtt_password);
  wm.setSaveParamsCallback(save_config);

  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(180);
  wm.setCaptivePortalEnable(true);

  if (wm.autoConnect(WIFI_CONFIG_AP_NAME)) {
    Serial.print("Connected to WiFi: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Populate the MAC address and setup the HomeAssistant device
    WiFi.macAddress(mac);
    Serial.print("MAC: "); for (auto v : mac) Serial.printf("\\x%.2x", v); Serial.println();
    device.setUniqueId(mac, sizeof(mac));
    device.setName("Status Sign");
    device.setManufacturer("V0rtex Labs");
    device.setSoftwareVersion(VERSION);
    device.enableSharedAvailability();
    device.enableLastWill();
    device.setAvailability(true);

    Serial.println("Home Assistant device setup complete");

    // Setup for the Select entity
    char** scene_list = new char*[NUM_SCENES];
    char options[NUM_SCENES * sizeof(Scene::name) + NUM_SCENES - 1];
    scene_names(scene_list);
    Serial.print("Scene names: ");
    for (int i = 0; i < NUM_SCENES; i++) {
      Serial.print(scene_list[i]);
      Serial.print(", ");
    }
    Serial.println();
    str_join(options, scene_list, NUM_SCENES, ";");
    Serial.printf("Options: %s\r\n", options);
    for (int i = 0; i < NUM_SCENES; i++) {
      delete[] scene_list[i];
    }
    delete[] scene_list;

    Select.setOptions(options);
    Select.onCommand([](int8_t index, HASelect* sender) {
      if (index < 0 || index >= NUM_SCENES) return;
      display_mode = Live;
      scene_manager.show(&scenes[index]);
      sender->setState(index);
    });
    Select.setIcon("mdi:message-star");
    Select.setName("Scene");
    mqtt.onConnected([]() { Serial.println("MQTT connected"); });
    mqtt.onMessage([](const char* topic, const uint8_t* payload, uint16_t length) {
      Serial.println("MQTT message received:");
      Serial.printf("  Topic: %s\r\n", topic);
      Serial.printf("  Payload: %s\r\n", payload);
    });
    mqtt.begin(
      mqtt_config.host,
      atoi(mqtt_config.port),
      mqtt_config.username,
      mqtt_config.password
    );
    Serial.println("MQTT setup complete");
  } else {
    Serial.print("Failed to connect to WiFi... configuration portal running with AP: ");
    Serial.println(wm.getConfigPortalSSID());
  }
}

// Main loop
void loop() {
  // Loop timer to show the time each loop takes
  // loopTimer.check(Serial);

  // Update the mDNS responder
  MDNS.update();

  // Let the wifi manager do its thing
  wm.process();

  // Update the MQTT client
  mqtt.loop();

  // Scroll the configuration SSID until the user connects
  if (WiFi.status() != WL_CONNECTED) {
    int16_t msg_color;
    char msg_text[40];
    if (wm.getConfigPortalActive()) {
      msg_color = display.color565(255, 128, 0); // Orange
      sprintf(msg_text, "SSID: %s", wm.getConfigPortalSSID());
    } else {
      msg_color = pxRED;
      sprintf(msg_text, "Config portal inactive... reset to try again");
    }
    scroll_text(
      (MATRIX_HEIGHT - TEXT_HEIGHT) / 2,  // Center vertically on the display
      50,                                 // Scroll delay in ms
      msg_text, msg_color
    );
  } else {
    if (!wm.getWebPortalActive()) {
      wm.startWebPortal();
    }

    // Handle demo mode
    if (display_mode == Demo) {
      scene_demo.loop();
    }
    scene_manager.loop();
  }
}