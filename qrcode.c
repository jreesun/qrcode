// Include required definitions first.
#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "qrcodegen.h"
#include "string.h"

STATIC mp_obj_t qrcode_encode(mp_obj_t text_obj) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(text_obj, &bufinfo, MP_BUFFER_READ);

    enum qrcodegen_Ecc ecl = qrcodegen_Ecc_LOW;  // Error correction level
    bool boostEcl = true;
    
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    memcpy(tempBuffer, bufinfo.buf, bufinfo.len);
    const char *text = bufinfo.buf;
    size_t textLen = strlen(text);
    bool is_bytes = false;
    if (textLen != bufinfo.len) {
        is_bytes = true;
    }
    struct qrcodegen_Segment seg;
    bool ok = true;
    if (textLen == 0) {
        ok = qrcodegen_encodeSegmentsAdvanced(NULL, 0, ecl, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, boostEcl, tempBuffer, qrcode);
    } else {
        if (!is_bytes) {
            size_t bufLen = (size_t)qrcodegen_BUFFER_LEN_FOR_VERSION(qrcodegen_VERSION_MAX);
            if (qrcodegen_isNumeric(text)) {
                if (qrcodegen_calcSegmentBufferSize(qrcodegen_Mode_NUMERIC, textLen) > bufLen) {
                    ok = false;
                } else {
                    seg = qrcodegen_makeNumeric(text, tempBuffer);
                }
            } else if (qrcodegen_isAlphanumeric(text)) {
                if (qrcodegen_calcSegmentBufferSize(qrcodegen_Mode_ALPHANUMERIC, textLen) > bufLen) {
                    ok = false;
                } else {
                    seg = qrcodegen_makeAlphanumeric(text, tempBuffer);
                }
            } else {
                is_bytes = true;
            }
        }
        if (is_bytes) {
            ok = qrcodegen_encodeBinary(
                tempBuffer,
                bufinfo.len,
                qrcode,
                ecl,
                qrcodegen_VERSION_MIN,
                qrcodegen_VERSION_MAX,
                qrcodegen_Mask_AUTO,
                boostEcl
            );
        } else if (ok) {
            ok = qrcodegen_encodeSegmentsAdvanced(&seg, 1, ecl, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, boostEcl, tempBuffer, qrcode);
        }
    }
    if (!ok) {
        mp_raise_ValueError("Failed to encode");
    }
    int size = qrcodegen_getSize(qrcode);

    size_t bufsize = (size * size + 7) / 8;
    vstr_t vstr;
    vstr_init_len(&vstr, bufsize);
    // Skip QR metadata byte at index 0
    memcpy(vstr.buf, &qrcode[1], bufsize - 1);
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qrcode_encode_obj, qrcode_encode);


/****************************** MODULE ******************************/

STATIC const mp_rom_map_elem_t qrcode_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_qrcode) },
    { MP_ROM_QSTR(MP_QSTR_encode), MP_ROM_PTR(&qrcode_encode_obj) },
};
STATIC MP_DEFINE_CONST_DICT(qrcode_module_globals, qrcode_module_globals_table);

// Define module object.
const mp_obj_module_t qrcode_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&qrcode_module_globals,
};

// Register the module to make it available in Python
MP_REGISTER_MODULE(MP_QSTR_qrcode, qrcode_user_cmodule, MODULE_QRCODE_ENABLED);
