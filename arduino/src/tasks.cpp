#include "tasks.h"

// -------------------------------
// Setup tasks
// -------------------------------
int32_t task_calibrate_ultra_sound_sensor(Ultra_Sound_Sensor* distance_sensor)
{
    int32_t min = distance_sensor_measure_distance(distance_sensor);
    int32_t current;
    for (uint8_t i = 0; i < CALIBRACTION_ITERATIONS - 1; ++i)
    {
        // We delay a little, because it seems that calling measure_distance
        // to rapidly, affects the results.
        delay(5); 

        //DEBUG_PRINTLN_VAR(min);
        current = distance_sensor_measure_distance(distance_sensor);

        if (current < min)
            min = current;
    }

    return min - 8;
}

// -------------------------------
// Task that are executed by the cyclic executive
// -------------------------------
void task_check_first_segment(Ultra_Sound_Sensor* distance_sensor, 
    uint16_t distance_to_wall, Segment_Queue* segment_queue)
{
    int32_t test_dist = distance_sensor_measure_distance(distance_sensor);

    DEBUG_PRINT_VAR(test_dist);
    DEBUG_PRINT(" ");
    DEBUG_PRINTLN_VAR(distance_to_wall);

    Segment* first_segment = queue_next(segment_queue);

    // Tests if a ball is in front of sensor
    if (test_dist < distance_to_wall)
    {
        //DEBUG_PRINTLN("Segment was occupied");
        first_segment->is_occupied = true;
        first_segment->object_type = BALL;
        first_segment->color = UNKNOWN;
    }
    else
    {
        //DEBUG_PRINTLN("Segment was empty");
        first_segment->is_occupied = false;
    }
}

void task_send_take_picture(Segment_Queue* queue)
{
    Segment* segment = get_segment(queue, KINECT_SEGMENT_INDEX);
    if (segment->is_occupied)
    {
        Out_Message message;
        message.type = OUT_MESSAGE_COMMAND;
        message.command.type = OUT_COMMAND_TAKE_PICURE;

        io_send_message(&message);
    }
}

void task_determin_color(SFE_ISL29125* color_sensor,
    Segment_Queue* segment_queue, Delta_RGB* known_colors)
{
    Segment* segment = get_segment(segment_queue, COLOR_SENSOR_SEGMENT_INDEX);

    if (segment->is_occupied && segment->object_type == BALL)
    {
        RGB color;
        read_color(color_sensor, &color);
        uint8_t determined_color = determin_color(known_colors, &color);
        segment->color = determined_color;

        //DEBUG_PRINT("Ball was: ");
        //DEBUG_PRINTLN(get_color_name(determined_color));
    }
}

void task_request_object_info(Segment_Queue* segment_queue)
{
    In_Message message;
    io_await_message(&message);

    if (message.type == IN_MESSAGE_OBJECT)
    {
        Segment* segment = 
            get_segment(segment_queue, KINECT_SEGMENT_INDEX);

        segment->object_type = message.object.type;
        return;
    }

    ASSERT(false);
}

void task_feed_ball(Motor* feeder)
{
    static uint8_t feed_counter = FEEDER_ITERATION;
    static int16_t deg = FEEDER_DEGREES;

    // We only feed a ball every x iterations
    if (feed_counter == FEEDER_ITERATION)
    {
        motor_turn_to_degree(feeder, deg);

        deg += FEEDER_DEGREES;
        if (deg == 360)
            deg = 0;

        feed_counter = 0;
    }

    feed_counter++;
}

void task_rotate_seperator(Advanced_Motor* separator, Segment_Queue* queue)
{
    static int16_t bucket_pos[BUCKET_COUNT] = { 
        RED_BUCKET, 
        GREEN_BUCKET, 
        BLUE_BUCKET, 
        YELLOW_BUCKET, 
        GARBAGE_BUCKET
    };

    Segment* segment = get_segment(queue, LAST_INDEX);
    static uint8_t last_position = GARBAGE_BUCKET;
    uint8_t position;

    if (segment->is_occupied && segment->object_type == BALL)
    {
        position = segment->color;
    }
    else
    {
        position = COLOR_COUNT;
    }

    if (position != last_position)
    {
        advanced_motor_turn_to_degree(separator, bucket_pos[position]);
        last_position = position;
    }
}