#include "CRoutineRun.h"

CRoutineRun::CRoutineRun(
        std::function<void(std::string)> send_function, 
        std::function<void(std::string result, int msg_count, std::string error)> send_ack_func,
        CRoutineOutput *routine_output,
        std::vector<CRoutines::Routine> *routines
)
{
    printf("CRoutineRun()\n");
    _send = send_function;
    _send_ack = send_ack_func;
    _routine_output = routine_output;
    _routines = routines;

    for (uint8_t channel=0; channel < MAX_CHANNELS; channel++)
    {
        _output_power[channel]     = _routine_output->get_output_power(channel);
        _max_output_power[channel] = _routine_output->get_max_output_power(channel);
        _front_panel_power[channel] = _routine_output->get_front_pannel_power(channel);
    }
}

CRoutineRun::~CRoutineRun()
{
    printf("~CRoutineRun()\n");
    _routine_output->stop_routine();

    for (uint8_t channel = 0; channel < MAX_CHANNELS; channel++)
        _routine_output->set_remote_power(channel, 0);
}

bool CRoutineRun::process(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc)
{
    std::string msgType = (*doc)["Type"];
    int msgCount = (*doc)["MsgCount"];
    bool pattern_start = false;

    if (msgType == "PatternStart")
    {
        int index = (*doc)["Index"];

        if (index < 0 || index >= (int)_routines->size())
        {
            send_ack("ERROR", msgCount);
            return true; // finished
        }

        _routine_output->activate_routine(index);
        pattern_start = true;
    }

    else if (msgType == "PatternMinMaxChange")
    {
        int menu_id   = (*doc)["MenuId"];
        int new_value = (*doc)["NewValue"];

        _routine_output->menu_min_max_change(menu_id, new_value);
    }

    else if (msgType == "PatternMultiChoiceChange")
    {
        int menu_id   = (*doc)["MenuId"];
        int choice_id = (*doc)["ChoiceId"];

        _routine_output->menu_multi_choice_change(menu_id, choice_id);
    }

    else if (msgType == "PatternSoftButton")
    {
        int pressed   = (*doc)["Pressed"];
        _routine_output->soft_button_pressed(soft_button::BUTTON_A, pressed != 0);
    }

    else if (msgType == "SetPower")
    {
        int channel_power[4] = {0};
        channel_power[0] = (*doc)["Chan1"];
        channel_power[1] = (*doc)["Chan2"];
        channel_power[2] = (*doc)["Chan3"];
        channel_power[3] = (*doc)["Chan4"];
        
        for (uint8_t channel = 0; channel < 4; channel++)
            _routine_output->set_remote_power(channel, channel_power[channel]);
    }

    send_ack("OK", msgCount);

    if (pattern_start)
        send_power_status_update();

    return false;
}

void CRoutineRun::loop()
{
    bool update_required = false;

    if (time_us_64() - _last_power_status_update_us > (250 * 1000)) // at most every 250ms
    {
        for(uint8_t channel = 0; channel < MAX_CHANNELS; channel++)
        {
            if (_routine_output->get_output_power(channel) != _output_power[channel])
            {
                _output_power[channel] = _routine_output->get_output_power(channel);
                update_required = true;
            }

            if (_routine_output->get_max_output_power(channel) != _max_output_power[channel])
            {
                _max_output_power[channel] = _routine_output->get_max_output_power(channel);
                update_required = true;
            }

            if (_routine_output->get_front_pannel_power(channel) != _front_panel_power[channel])
            {
                _front_panel_power[channel] = _routine_output->get_front_pannel_power(channel);
                update_required = true;
            }
        }

        if (update_required)
        {
            send_power_status_update();
            _last_power_status_update_us = time_us_64();
        }
    }
}

void CRoutineRun::send_power_status_update()
{
    StaticJsonDocument<1000> status_message;

    status_message["Type"] = "PowerStatus";
    status_message["MsgCount"] = -1;

    JsonArray channels = status_message.createNestedArray("Channels");
    for(uint8_t channel = 0; channel < MAX_CHANNELS; channel++)
    {
        JsonObject obj = channels.createNestedObject();
        obj["Channel"]        = channel+1;
        obj["OutputPower"]    = _output_power[channel];
        obj["MaxOutputPower"] = _max_output_power[channel];
        obj["PowerLimit"]     = _front_panel_power[channel];
    }

    std::string generatedJson;
    serializeJson(status_message, generatedJson);
    _send(generatedJson);
}

void CRoutineRun::send_ack(std::string result, int msg_count)
{
    _send_ack(result, msg_count, "");
}
