#include <TensorFlowLite.h>
#include <Wire.h>
#include <Arduino_LSM9DS1.h>
#include <Arduino_BMI270_BMM150.h>
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
#include "gesture_model_data.h"

BoschSensorClass IMU(Wire1);
LSM9DS1Class IMUExternal(Wire);

tflite::MicroErrorReporter micro_error_reporter;
tflite::ErrorReporter* error_reporter = &micro_error_reporter;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

constexpr int kTensorArenaSize = 90 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

constexpr int label_count = 7;
const char* labels[label_count] = {"copy", "paste", "undo", "yes", "no", "unknown", "not moving"};

constexpr int n_features = 12;
constexpr int n_timesteps = 75;
float data_buffer[n_timesteps][n_features];

const float input_scale = 0.09062974154949188;
const int input_zero_point = -12;

inline int8_t quantize(float x) {
  return static_cast<int8_t>(x / input_scale + input_zero_point);
}

float standard[12][2] = {
  {-0.09188928914505284, 0.27918700026123083},  // Ax
  {-0.010937896253602305, 0.16237554396106005}, // Ay
  {0.9294043707973104, 0.3014870384791123},     // Az
  {0.2829411623439001, 37.809284184568206},     // Gx
  {0.3697540257648954, 95.72956135970946},       // Gy
  {-0.31483885686839586, 23.411161467952585},   // Gz
  {0.11876109510086454, 0.48686742814469935},      // AEx
  {-0.5355396733909702, 0.33571267449846676},    // AEy
  {-0.6793276176753122, 0.47743214018159524},    // AEz
  {2.958989000960614, 94.68613190402625},      // GEx
  {2.5286330451488954, 109.26055642679748},      // GEy
  {0.19222094140249765, 64.50730201721385}      // GEz
};


unsigned long start_time = 0; // Start time for the 2-second window
constexpr unsigned long time_window = 2000; // 2-second time window
int label_counts[label_count] = {0}; // Array to count label occurrences
bool recognizing = false; // Flag to indicate recognition period

void normalize(float &ax, float &ay, float &az, float &gx, float &gy, float &gz, float &aex, float &aey, float &aez, float &gex, float &gey, float &gez) {
  ax = (ax - standard[0][0]) / standard[0][1];
  ay = (ay - standard[1][0]) / standard[1][1];
  az = (az - standard[2][0]) / standard[2][1];

  gx = (gx - standard[3][0]) / standard[3][1];
  gy = (gy - standard[4][0]) / standard[4][1];
  gz = (gz - standard[5][0]) / standard[5][1];

  aex = (aex - standard[6][0]) / standard[6][1];
  aey = (aey - standard[7][0]) / standard[7][1];
  aez = (aez - standard[8][0]) / standard[8][1];

  gex = (gex - standard[9][0]) / standard[9][1];
  gey = (gey - standard[10][0]) / standard[10][1];
  gez = (gez - standard[11][0]) / standard[11][1];
}

void setup() {
  Serial.begin(9600);
  Serial.println("Setup started");

  if (!IMU.begin()) {
    Serial.println("Failed to initialize internal IMU!");
    while (1);
  } else {
    Serial.println("Internal IMU initialized");
  }
  
  if (!IMUExternal.begin()) {
    Serial.println("Failed to initialize external IMU!");
    while (1);
  } else {
    Serial.println("External IMU initialized");
  }

  model = tflite::GetModel(gesture_model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model version does not match Schema version.");
    while (1);
  } else {
    Serial.println("Model loaded successfully");
  }

  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("AllocateTensors() failed");
    while (1);
  } else {
    Serial.println("Tensors allocated successfully");
  }

  input = interpreter->input(0);
  output = interpreter->output(0);

  Serial.println("Setup completed");
}

void loop() {
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable() && IMUExternal.accelerationAvailable() && IMUExternal.gyroscopeAvailable()) {
    float ax, ay, az, gx, gy, gz;
    float aex, aey, aez, gex, gey, gez;
    
    IMU.readAcceleration(ax, ay, az);
    IMU.readGyroscope(gx, gy, gz);
    IMUExternal.readAcceleration(aex, aey, aez);
    IMUExternal.readGyroscope(gex, gey, gez);
    
    normalize(ax, ay, az, gx, gy, gz, aex, aey, aez, gex, gey, gez);

    for (int i = 0; i < n_timesteps - 1; i++) {
      for (int j = 0; j < n_features; j++) {
        data_buffer[i][j] = data_buffer[i + 1][j];
      }
    }

    data_buffer[n_timesteps - 1][0] = ax;
    data_buffer[n_timesteps - 1][1] = ay;
    data_buffer[n_timesteps - 1][2] = az;
    data_buffer[n_timesteps - 1][3] = gx;
    data_buffer[n_timesteps - 1][4] = gy;
    data_buffer[n_timesteps - 1][5] = gz;
    data_buffer[n_timesteps - 1][6] = aex;
    data_buffer[n_timesteps - 1][7] = aey;
    data_buffer[n_timesteps - 1][8] = aez;
    data_buffer[n_timesteps - 1][9] = gex;
    data_buffer[n_timesteps - 1][10] = gey;
    data_buffer[n_timesteps - 1][11] = gez;

    for (int i = 0; i < n_timesteps; i++) {
      for (int j = 0; j < n_features; j++) {
        input->data.int8[i * n_features + j] = quantize(data_buffer[i][j]);
      }
    }

    if (interpreter->Invoke() != kTfLiteOk) {
      Serial.println("Invoke failed");
      return;
    }

    int8_t max_score = -128;  // Initializing to the minimum possible value for int8_t
    int max_index = 0;
    bool found_valid_score = false;

    // Loop to find the max score excluding label 5
    for (int i = 0; i < label_count; i++) {
        if (i == 1 && output->data.int8[i] > -110) {
          if (output->data.int8[i] > max_score) {
              max_score = output->data.int8[i];
              max_index = i;
              found_valid_score = true;
          }
        }
        else if (i == 0 && output->data.int8[i] > -80) {
          if (output->data.int8[i] > max_score) {
              max_score = output->data.int8[i];
              max_index = i;
              found_valid_score = true;
          }
        }
        else if (i != 0 && i != 1 && i != 5 && output->data.int8[i] > -90) {
          if (output->data.int8[i] > max_score) {
              max_score = output->data.int8[i];
              max_index = i;
              found_valid_score = true;
          }
        }
    }

    if (found_valid_score) {
      unsigned long current_time = millis();
      if (!recognizing) {
        // Start recognizing period
        recognizing = true;
        start_time = current_time;
        memset(label_counts, 0, sizeof(label_counts));
      }

      // Count the recognized label
      label_counts[max_index]++;

      // Check if the 2-second window has passed
      if (current_time - start_time >= time_window) {
        // Find the most frequent label
        int most_frequent_label = 0;
        int highest_count = 0;
        for (int i = 0; i < label_count; i++) {
          if (label_counts[i] > highest_count) {
            highest_count = label_counts[i];
            most_frequent_label = i;
          }
        }

        // Print the most frequent label
        Serial.println(labels[most_frequent_label]);

        // Reset recognizing period
        recognizing = false;
      }
    }
  } else {
    Serial.println("IMU data not available");
  }
}
