#pragma once

#include <map>
#include <memory>
#include <string>

class BME280 {
public:
    enum Mode : uint8_t {
        SLEEP = 0,
        FORCED = 1,
        NORMAL = 3,
    };

    enum Filter : uint8_t {
        OFF = 0,
        _2 = 1,
        _4 = 2,
        _8 = 3,
        _16 = 4
    };

    // Pressure oversampling settings
    enum Oversampling : uint8_t {
        SKIPPED = 0, ///< no measurement
        ULTRA_LOW_POWER = 1, ///< oversampling x1
        LOW_POWER = 2, ///< oversampling x2
        STANDARD = 3, ///< oversampling x4
        HIGH_RES = 4, ///< oversampling x8
        ULTRA_HIGH_RES = 5 ///< oversampling x16
    };

    // Stand by time between measurements in normal mode
    enum StandbyTime : uint8_t {
        _05ms = 0, ///< stand by time 0.5ms
        _62ms = 1, ///< stand by time 62.5ms
        _125ms = 2, ///< stand by time 125ms
        _250ms = 3, ///< stand by time 250ms
        _500ms = 4, ///< stand by time 500ms
        _1000ms = 5, ///< stand by time 1s
        _2000ms = 6, ///< stand by time 2s BME280, 10ms BME280
        _4000ms = 7, ///< stand by time 4s BME280, 20ms BME280
    };

    // Configuration parameters for BME280 module.
    // Use function bme280_init_default_params to use default configuration.
    struct Config {
        Mode mode;
        Filter filter;
        Oversampling oversamplingPressure;
        Oversampling oversamplingTemperature;
        Oversampling oversamplingHumidity;
        StandbyTime standby;
    };

    inline static constexpr Config DEFAULT_CONFIGURATION{
        Mode::NORMAL,
        Filter::OFF,
        Oversampling::STANDARD,
        Oversampling::STANDARD,
        Oversampling::STANDARD,
        StandbyTime::_500ms,
    };

    static std::shared_ptr<BME280> instance(const std::string_view file, uint16_t address, Config config = DEFAULT_CONFIGURATION);
    ~BME280();

    void open(); // In fun. instance will open
    bool opened() const noexcept;
    void check() const;
    void close();

    void setConfig(Config config);

    void softReset() const;

    void calibration();

    bool wasCalibration() const noexcept;

    time_t getTimeLastCalibration() const noexcept;

    float getTemperature() const; // Get real temperature

    float getHumidity() const; // Get real humidity

    uint32_t getQnhPressure(double altitude) const; // Returns the QNH pressure at sea level at the measuring point in ppa
    uint32_t getQfePressure() const; // Get the QFE pressure at the measuring point in ppa

    static float paToHg(float ppa); // Fast integer Pa -> mmHg conversion (Pascals to millimeters of mercury)
    static float calcDewpoint(float humidity, float temperature); // Get the dew point in celsius

private:
    // Calibration parameters structure
    struct Calibration {
        uint16_t T1 = 0;
        int16_t T2 = 0;
        int16_t T3 = 0;

        uint16_t P1 = 0;
        int16_t P2 = 0;
        int16_t P3 = 0;
        int16_t P4 = 0;
        int16_t P5 = 0;
        int16_t P6 = 0;
        int16_t P7 = 0;
        int16_t P8 = 0;
        int16_t P9 = 0; //18

        // Humidity compensation for BME280
        uint8_t H1 = 0;
        int16_t H2 = 0;
        uint8_t H3 = 0;
        int16_t H4 = 0;
        int16_t H5 = 0;
        int8_t H6 = 0;

        time_t updateTime = 0;
    };

    enum Registers : uint8_t {
        TEMP_PRESS_CALIB_DATA_ADDR = 0x88, ///< E2PROM calibration data start register
        CHIP_ID_REG = 0xD0, ///< Chip ID
        HUM_LSB = 0xFE,
        HUM_MSB = 0xFD,
        TEMP_XLSB = 0xFC, ///< bits: 7-4
        TEMP_LSB = 0xFB,
        TEMP_MSB = 0xFA,
        TEMP = TEMP_MSB,
        PRESS_XLSB = 0xF9, ///< bits: 7-4
        PRESS_LSB = 0xF8,
        PRESS_MSB = 0xF7,
        PRESSURE = PRESS_MSB,
        CONFIG = 0xF5, ///< bits: 7-5 t_sb; 4-2 filter; 0 spi3w_en
        CTRL = 0xF4, ///< bits: 7-5 osrs_t; 4-2 osrs_p; 1-0 mode
        STATUS = 0xF3, ///< bits: 3 measuring; 0 im_update
        CTRL_HUM = 0xF2, ///< bits: 2-0 osrs_h;
        HUM_CALIB_H1 = 0xA1,
        HUM_CALIB_H2_LSB = 0xE1,
        HUM_CALIB_H2_MSB = 0xE2,
        HUM_CALIB_H3 = 0xE3,
        HUM_CALIB_H4_MSB = 0xE5, ///< H4[11:4 3:0] = 0xE4[7:0] 0xE5[3:0] 12-bit signed
        HUM_CALIB_H4_LSB = 0xE4,
        HUM_CALIB_H5_MSB = 0xE6, ///< H5[11:4 3:0] = 0xE6[7:0] 0xE5[7:4] 12-bit signed
        HUM_CALIB_H5_LSB = 0xE5,
        HUM_CALIB_H6 = 0xE7,
        SOFT_RESET_REG = 0xE0, ///< Soft reset control
    };

    static constexpr uint16_t TIMEOUT_FOR_SOFT_RESET = 1000; ///< In milliseconds
    static constexpr size_t TEMP_PRESS_CALIB_DATA_LEN = 24;
    static constexpr int CLOSE = -1;

    static constexpr uint8_t CHIP_ID = 0x60;
    static constexpr uint8_t SOFT_RESET_COMMAND = 0xB6;
    static constexpr uint8_t STATUS_OK = 0;

    explicit BME280(const std::string_view device);

    int32_t getRawTemperature() const;
    int32_t calcTemperature(int32_t raw) const;
    int32_t getFineTemperature() const;

    uint32_t readRawHumidity() const;
    uint32_t calcHumidity(uint32_t raw) const;

    int32_t getRawPressure() const;
    uint32_t calcPressure(int32_t raw) const;

    void access(uint8_t markers, uint8_t command, uint32_t size, union i2c_smbus_data* data) const;
    void writedByteData(uint8_t registers, uint8_t value) const;
    uint8_t readByteData(uint8_t registers) const;
    size_t readBlockData(uint8_t registers, uint8_t* values, size_t length) const;

    const std::string _device;
    uint16_t _address;
    int _i2c;

    Config _config;
    Calibration _calibration;

    inline static std::map<std::string, std::weak_ptr<BME280>> _interfaces;
};