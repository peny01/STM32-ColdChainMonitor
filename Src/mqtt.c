#include "mqtt.h"
#include "esp8266.h"

/* MQTT Broker */
#define MQTT_BROKER_IP        "broker.emqx.io"
#define MQTT_BROKER_PORT      1883

/* MQTT报文类型 */
#define MQTT_CONNECT          0x10
#define MQTT_CONNACK          0x20
#define MQTT_PUBLISH          0x30
#define MQTT_SUBSCRIBE        0x82
#define MQTT_SUBACK           0x90
#define MQTT_DISCONNECT       0xE0

/* MQTT协议版本 3.1.1 = 4 */
#define MQTT_PROTOCOL_LEVEL   4

static uint8_t mqtt_connected = 0;
static uint16_t packet_id = 1;
static uint8_t mqtt_pkt_buf[MQTT_BUF_SIZE];  /* static to avoid stack overflow */

/* 内部函数声明 */
static uint8_t MQTT_EncodeRemLen(uint32_t length, uint8_t *buf);
static uint8_t MQTT_SendConnect(const char *client_id, const char *username, const char *password);

/* 编码剩余长度（可变字节） */
static uint8_t MQTT_EncodeRemLen(uint32_t length, uint8_t *buf)
{
    uint8_t count = 0;
    do {
        buf[count] = length % 128;
        length /= 128;
        if (length > 0)
            buf[count] |= 0x80;
        count++;
    } while (length > 0);
    return count;
}

/* 内部：构建并发送CONNECT报文 */
static uint8_t MQTT_SendConnect(const char *client_id, const char *username, const char *password)
{
    uint8_t remlen_buf[4];
    uint16_t pos = 1;          /* 跳过固定头第1字节 */
    uint32_t remlen;
    uint8_t remlen_cnt;
    uint8_t connect_flags = 0x02;  /* Clean Session */

    /* === 可变头 === */
    /* 协议名 "MQTT"（2字节长度 + 4字节数据） */
    mqtt_pkt_buf[pos++] = 0x00;
    mqtt_pkt_buf[pos++] = 0x04;
    mqtt_pkt_buf[pos++] = 'M';
    mqtt_pkt_buf[pos++] = 'Q';
    mqtt_pkt_buf[pos++] = 'T';
    mqtt_pkt_buf[pos++] = 'T';

    /* 协议级别 */
    mqtt_pkt_buf[pos++] = MQTT_PROTOCOL_LEVEL;

    /* 连接标志 */
    if (username != NULL && strlen(username) > 0)
        connect_flags |= 0x80;  /* Username flag */
    if (password != NULL && strlen(password) > 0)
        connect_flags |= 0x40;  /* Password flag */
    mqtt_pkt_buf[pos++] = connect_flags;

    /* Keep Alive: 60秒 */
    mqtt_pkt_buf[pos++] = 0x00;
    mqtt_pkt_buf[pos++] = 0x3C;

    /* === 有效载荷 === */
    /* Client ID */
    {
        uint16_t len = (client_id != NULL) ? strlen(client_id) : 0;
        mqtt_pkt_buf[pos++] = (len >> 8) & 0xFF;
        mqtt_pkt_buf[pos++] = len & 0xFF;
        if (len > 0) {
            memcpy(&mqtt_pkt_buf[pos], client_id, len);
            pos += len;
        }
    }

    /* Username */
    if (connect_flags & 0x80)
    {
        uint16_t len = strlen(username);
        mqtt_pkt_buf[pos++] = (len >> 8) & 0xFF;
        mqtt_pkt_buf[pos++] = len & 0xFF;
        memcpy(&mqtt_pkt_buf[pos], username, len);
        pos += len;
    }

    /* Password */
    if (connect_flags & 0x40)
    {
        uint16_t len = strlen(password);
        mqtt_pkt_buf[pos++] = (len >> 8) & 0xFF;
        mqtt_pkt_buf[pos++] = len & 0xFF;
        memcpy(&mqtt_pkt_buf[pos], password, len);
        pos += len;
    }

    /* 计算并插入剩余长度编码 */
    remlen = pos - 1;                          /* 可变头+有效载荷总长 */
    remlen_cnt = MQTT_EncodeRemLen(remlen, remlen_buf);

    /* 将可变头+有效载荷后移，给remlen编码腾位置 */
    memmove(&mqtt_pkt_buf[1 + remlen_cnt], &mqtt_pkt_buf[1], (size_t)remlen);
    memcpy(&mqtt_pkt_buf[1], remlen_buf, remlen_cnt);

    mqtt_pkt_buf[0] = MQTT_CONNECT;                 /* 固定头第1字节 */

    uint16_t total_len = 1 + remlen_cnt + (uint16_t)remlen;

    /* 通过ESP8266发送 */
    if (ESP8266_SendData(mqtt_pkt_buf, total_len) != 0)
        return 1;

    /* 等待接收CONNACK */
    HAL_Delay(500);

    return 0;
}

/* MQTT连接 */
uint8_t MQTT_Connect(const char *client_id, const char *username, const char *password)
{
    /* 1. 先建立TCP连接到MQTT Broker */
    if (ESP8266_TCP_Connect(MQTT_BROKER_IP, MQTT_BROKER_PORT) != 0)
        return 1;

    HAL_Delay(500);

    /* 2. 发送CONNECT报文 */
    if (MQTT_SendConnect(client_id, username, password) != 0)
    {
        ESP8266_CloseTCP();
        return 2;
    }

    mqtt_connected = 1;
    return 0;
}

/* MQTT断开连接 */
uint8_t MQTT_Disconnect(void)
{
    if (!mqtt_connected) return 0;

    /* 发送DISCONNECT报文（固定头0xE0 + 剩余长度0） */
    uint8_t disc_buf[2] = {MQTT_DISCONNECT, 0x00};
    ESP8266_SendData(disc_buf, 2);

    HAL_Delay(200);
    ESP8266_CloseTCP();
    mqtt_connected = 0;

    return 0;
}

/* MQTT订阅主题 (QoS 0) */
uint8_t MQTT_Subscribe(const char *topic)
{
    uint8_t remlen_buf[4];
    uint16_t pos = 1;
    uint32_t remlen;
    uint8_t remlen_cnt;
    uint16_t topic_len;

    if (!mqtt_connected) return 1;

    /* Packet Identifier */
    mqtt_pkt_buf[pos++] = (packet_id >> 8) & 0xFF;
    mqtt_pkt_buf[pos++] = packet_id & 0xFF;

    /* Topic Filter */
    topic_len = strlen(topic);
    mqtt_pkt_buf[pos++] = (topic_len >> 8) & 0xFF;
    mqtt_pkt_buf[pos++] = topic_len & 0xFF;
    memcpy(&mqtt_pkt_buf[pos], topic, topic_len);
    pos += topic_len;

    /* Requested QoS */
    mqtt_pkt_buf[pos++] = 0x00;

    remlen = pos - 1;
    remlen_cnt = MQTT_EncodeRemLen(remlen, remlen_buf);

    memmove(&mqtt_pkt_buf[1 + remlen_cnt], &mqtt_pkt_buf[1], (size_t)remlen);
    memcpy(&mqtt_pkt_buf[1], remlen_buf, remlen_cnt);
    mqtt_pkt_buf[0] = MQTT_SUBSCRIBE;

    uint16_t total_len = 1 + remlen_cnt + (uint16_t)remlen;

    if (ESP8266_SendData(mqtt_pkt_buf, total_len) != 0)
        return 2;

    packet_id++;
    return 0;
}

/* MQTT取消订阅 */
uint8_t MQTT_Unsubscribe(const char *topic)
{
    (void)topic;
    /* 简化实现 */
    return 0;
}

/* MQTT发布消息 */
uint8_t MQTT_Publish(const char *topic, const char *payload, uint8_t qos)
{
    uint8_t remlen_buf[4];
    uint16_t pos = 1;
    uint32_t remlen;
    uint8_t remlen_cnt;
    uint16_t topic_len;
    uint16_t payload_len;
    uint8_t header_type;

    if (!mqtt_connected) return 1;

    /* 固定头类型 */
    header_type = MQTT_PUBLISH | ((qos & 0x03) << 1);

    /* Topic */
    topic_len = strlen(topic);
    mqtt_pkt_buf[pos++] = (topic_len >> 8) & 0xFF;
    mqtt_pkt_buf[pos++] = topic_len & 0xFF;
    memcpy(&mqtt_pkt_buf[pos], topic, topic_len);
    pos += topic_len;

    /* Packet Identifier (仅QoS >= 1) */
    if (qos > 0)
    {
        mqtt_pkt_buf[pos++] = (packet_id >> 8) & 0xFF;
        mqtt_pkt_buf[pos++] = packet_id & 0xFF;
        packet_id++;
    }

    /* Payload */
    payload_len = strlen(payload);
    memcpy(&mqtt_pkt_buf[pos], payload, payload_len);
    pos += payload_len;

    remlen = pos - 1;
    remlen_cnt = MQTT_EncodeRemLen(remlen, remlen_buf);

    memmove(&mqtt_pkt_buf[1 + remlen_cnt], &mqtt_pkt_buf[1], (size_t)remlen);
    memcpy(&mqtt_pkt_buf[1], remlen_buf, remlen_cnt);
    mqtt_pkt_buf[0] = header_type;

    uint16_t total_len = 1 + remlen_cnt + (uint16_t)remlen;

    if (ESP8266_SendData(mqtt_pkt_buf, total_len) != 0)
        return 2;

    return 0;
}

/* 发布传感器数据JSON到 coldchain/data */
uint8_t MQTT_PublishSensorData(SensorData_t *data)
{
    char json[MQTT_BUF_SIZE];

    if (data == NULL) return 1;

    snprintf(json, sizeof(json),
        "{\"temp\":%.1f,\"hum\":%.1f,\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f,"
        "\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f,\"speed\":%.1f,"
        "\"gps_fix\":%u,\"sat\":%u,\"ts\":%lu}",
        data->temperature, data->humidity,
        data->accel_x, data->accel_y, data->accel_z,
        data->latitude, data->longitude, data->altitude, data->speed,
        data->gps_fix, data->gps_satellites, (unsigned long)data->timestamp);

    return MQTT_Publish("coldchain/data", json, 0);
}

/* 发布报警到 coldchain/alarm */
uint8_t MQTT_PublishAlarm(const char *alarm_type, const char *message)
{
    char json[MQTT_BUF_SIZE];

    snprintf(json, sizeof(json),
        "{\"type\":\"%s\",\"msg\":\"%s\",\"ts\":%lu}",
        alarm_type, message, (unsigned long)(HAL_GetTick() / 1000));

    return MQTT_Publish("coldchain/alarm", json, 0);
}

/* 处理收到的MQTT报文（轮询调用） */
uint8_t MQTT_ProcessIncoming(void)
{
    return 0;
}
