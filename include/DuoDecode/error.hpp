#pragma once
#include <cerrno>
#include <cstdint>
#include <result/declare_error_code.h>
#include <result/result_type.hpp>
#include <system_error>
#include <result.hpp>
extern "C" {
#include <libavutil/error.h>
}


namespace dd {
    enum class errc : std::int32_t {
        __posix_end = EHWPOISON,
        inconsistent_video_format,
        invalid_audio_channel_order,
        decoder_not_open,
        num_codes = 0,

        //avcodec error codes
        bitstream_filter_not_found   = -AVERROR_BSF_NOT_FOUND,
        internal_bug                 = -AVERROR_BUG,
        buffer_too_small             = -AVERROR_BUFFER_TOO_SMALL,
        decoder_not_found            = -AVERROR_DECODER_NOT_FOUND,
        demuxer_not_found            = -AVERROR_DEMUXER_NOT_FOUND,
        encoder_not_found            = -AVERROR_ENCODER_NOT_FOUND,
        end_of_file                  = -AVERROR_EOF,
        immediate_exit_requested     = -AVERROR_EXIT,
        external_library_error       = -AVERROR_EXTERNAL,
        filter_not_found             = -AVERROR_FILTER_NOT_FOUND,
        invalid_input_data           = -AVERROR_INVALIDDATA,
        muxer_not_found              = -AVERROR_MUXER_NOT_FOUND,
        option_not_found             = -AVERROR_OPTION_NOT_FOUND,
        not_yet_implemented          = -AVERROR_PATCHWELCOME,
        protocol_not_found           = -AVERROR_PROTOCOL_NOT_FOUND,
        stream_not_found             = -AVERROR_STREAM_NOT_FOUND,
        unknown                      = -AVERROR_UNKNOWN,
        experimental_feature         = -AVERROR_EXPERIMENTAL,
        input_changed_between_calls  = -AVERROR_INPUT_CHANGED,
        output_changed_between_calls = -AVERROR_OUTPUT_CHANGED,


        //posix error codes
        address_family_not_supported       = static_cast<std::int32_t>(std::errc::address_family_not_supported),
        address_in_use                     = static_cast<std::int32_t>(std::errc::address_in_use),
        address_not_available              = static_cast<std::int32_t>(std::errc::address_not_available),
        already_connected                  = static_cast<std::int32_t>(std::errc::already_connected),
        argument_list_too_long             = static_cast<std::int32_t>(std::errc::argument_list_too_long),
        argument_out_of_domain             = static_cast<std::int32_t>(std::errc::argument_out_of_domain),
        bad_address                        = static_cast<std::int32_t>(std::errc::bad_address),
        bad_file_descriptor                = static_cast<std::int32_t>(std::errc::bad_file_descriptor),
        bad_message                        = static_cast<std::int32_t>(std::errc::bad_message),
        broken_pipe                        = static_cast<std::int32_t>(std::errc::broken_pipe),
        connection_aborted                 = static_cast<std::int32_t>(std::errc::connection_aborted),
        connection_already_in_progress     = static_cast<std::int32_t>(std::errc::connection_already_in_progress),
        connection_refused                 = static_cast<std::int32_t>(std::errc::connection_refused),
        connection_reset                   = static_cast<std::int32_t>(std::errc::connection_reset),
        cross_device_link                  = static_cast<std::int32_t>(std::errc::cross_device_link),
        destination_address_required       = static_cast<std::int32_t>(std::errc::destination_address_required),
        device_or_resource_busy            = static_cast<std::int32_t>(std::errc::device_or_resource_busy),
        directory_not_empty                = static_cast<std::int32_t>(std::errc::directory_not_empty),
        executable_format_error            = static_cast<std::int32_t>(std::errc::executable_format_error),
        file_exists                        = static_cast<std::int32_t>(std::errc::file_exists),
        file_too_large                     = static_cast<std::int32_t>(std::errc::file_too_large),
        filename_too_long                  = static_cast<std::int32_t>(std::errc::filename_too_long),
        function_not_supported             = static_cast<std::int32_t>(std::errc::function_not_supported),
        host_unreachable                   = static_cast<std::int32_t>(std::errc::host_unreachable),
        identifier_removed                 = static_cast<std::int32_t>(std::errc::identifier_removed),
        illegal_byte_sequence              = static_cast<std::int32_t>(std::errc::illegal_byte_sequence),
        inappropriate_io_control_operation = static_cast<std::int32_t>(std::errc::inappropriate_io_control_operation),
        interrupted                        = static_cast<std::int32_t>(std::errc::interrupted),
        invalid_argument                   = static_cast<std::int32_t>(std::errc::invalid_argument),
        invalid_seek                       = static_cast<std::int32_t>(std::errc::invalid_seek),
        io_error                           = static_cast<std::int32_t>(std::errc::io_error),
        is_a_directory                     = static_cast<std::int32_t>(std::errc::is_a_directory),
        message_size                       = static_cast<std::int32_t>(std::errc::message_size),
        network_down                       = static_cast<std::int32_t>(std::errc::network_down),
        network_reset                      = static_cast<std::int32_t>(std::errc::network_reset),
        network_unreachable                = static_cast<std::int32_t>(std::errc::network_unreachable),
        no_buffer_space                    = static_cast<std::int32_t>(std::errc::no_buffer_space),
        no_child_process                   = static_cast<std::int32_t>(std::errc::no_child_process),
        no_link                            = static_cast<std::int32_t>(std::errc::no_link),
        no_lock_available                  = static_cast<std::int32_t>(std::errc::no_lock_available),
        no_message                         = static_cast<std::int32_t>(std::errc::no_message),
        no_protocol_option                 = static_cast<std::int32_t>(std::errc::no_protocol_option),
        no_space_on_device                 = static_cast<std::int32_t>(std::errc::no_space_on_device),
        no_such_device_or_address          = static_cast<std::int32_t>(std::errc::no_such_device_or_address),
        no_such_device                     = static_cast<std::int32_t>(std::errc::no_such_device),
        no_such_file_or_directory          = static_cast<std::int32_t>(std::errc::no_such_file_or_directory),
        no_such_process                    = static_cast<std::int32_t>(std::errc::no_such_process),
        not_a_directory                    = static_cast<std::int32_t>(std::errc::not_a_directory),
        not_a_socket                       = static_cast<std::int32_t>(std::errc::not_a_socket),
        not_connected                      = static_cast<std::int32_t>(std::errc::not_connected),
        not_enough_memory                  = static_cast<std::int32_t>(std::errc::not_enough_memory),
        not_supported                      = static_cast<std::int32_t>(std::errc::not_supported),
        operation_canceled                 = static_cast<std::int32_t>(std::errc::operation_canceled),
        operation_in_progress              = static_cast<std::int32_t>(std::errc::operation_in_progress),
        operation_not_permitted            = static_cast<std::int32_t>(std::errc::operation_not_permitted),
        operation_not_supported            = static_cast<std::int32_t>(std::errc::operation_not_supported),
        operation_would_block              = static_cast<std::int32_t>(std::errc::operation_would_block),
        owner_dead                         = static_cast<std::int32_t>(std::errc::owner_dead),
        permission_denied                  = static_cast<std::int32_t>(std::errc::permission_denied),
        protocol_error                     = static_cast<std::int32_t>(std::errc::protocol_error),
        protocol_not_supported             = static_cast<std::int32_t>(std::errc::protocol_not_supported),
        read_only_file_system              = static_cast<std::int32_t>(std::errc::read_only_file_system),
        resource_deadlock_would_occur      = static_cast<std::int32_t>(std::errc::resource_deadlock_would_occur),
        resource_unavailable_try_again     = static_cast<std::int32_t>(std::errc::resource_unavailable_try_again),
        result_out_of_range                = static_cast<std::int32_t>(std::errc::result_out_of_range),
        state_not_recoverable              = static_cast<std::int32_t>(std::errc::state_not_recoverable),
        text_file_busy                     = static_cast<std::int32_t>(std::errc::text_file_busy),
        timed_out                          = static_cast<std::int32_t>(std::errc::timed_out),
        too_many_files_open_in_system      = static_cast<std::int32_t>(std::errc::too_many_files_open_in_system),
        too_many_files_open                = static_cast<std::int32_t>(std::errc::too_many_files_open),
        too_many_links                     = static_cast<std::int32_t>(std::errc::too_many_links),
        too_many_symbolic_link_levels      = static_cast<std::int32_t>(std::errc::too_many_symbolic_link_levels),
        value_too_large                    = static_cast<std::int32_t>(std::errc::value_too_large),
        wrong_protocol_type                = static_cast<std::int32_t>(std::errc::wrong_protocol_type),
    };
}

OL_RESULT_DECLARE_AS_ERROR_CODE(dd, errc, nullptr, nullptr, duodecode);


namespace dd {
    template<typename T>
    using result = ol::result<T, dd::errc>;
}