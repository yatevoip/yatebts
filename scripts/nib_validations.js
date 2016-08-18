function checkValidKi(error,field_name,field_value,section_name)
{
    if (field_value == "*")
	return true;

    if (!checkValidHex(error,field_name,field_value,section_name))
	return false;

    return true;
}

function checkValidHex(error,field_name,field_value,section_name)
{
    var str = /^[0-9a-fA-F]{32}$/;
    if (!str.test(field_value)) {
	error.reason =  "Invalid value '" + field_value + "' for '" + field_name + "' in section  '" + section_name + "'. Field must be 128 bits in hex format.";
	error.error = 401;
	return false;
    }
    return true;
}

function checkValidIccid(error,field_name,field_value,section_name)
{
    if (isNaN(parseInt(field_value)) || field_value.length != 19) {
	error.reason = "ICCID: '". field_value ."' must contain 19 digits.";
	error.error = 401;
	return false;
    }
    return true;
}
