use h5f::H5F_ACC_RDONLY;
use hdf_eos5_sys::h5p::H5P_DEFAULT;
pub use hdf_eos5_sys::*;

pub fn open_hdf5_file(filename: &str) -> hid_t {
    // Convert to C string
    let data = std::ffi::CString::new(filename).unwrap();
    // Convert data to *const c_char
    let fileid = unsafe { H5Fopen(data.as_ptr(), 
        H5F_ACC_RDONLY,
        H5P_DEFAULT) };

    println!("File ID: {}", fileid);

    if fileid < 0 {
        panic!("Error opening file");
    }

    fileid
}

pub fn open_file(filename: &str) -> hid_t {
    // Convert to C string
    let data = std::ffi::CString::new(filename).unwrap();
    // Convert data to *const c_char
    let fileid = unsafe { HE5_EHopen(data.as_ptr(), 
        H5F_ACC_RDONLY,
        H5P_DEFAULT) };

    println!("File ID: {}", fileid);

    if fileid < 0 {
        panic!("Error opening file");
    }

    fileid
}

// HE5_GDgetaliaslist
pub fn get_alias_list(fileid: hid_t) -> Vec<String> {
    let mut alias_count = 0;
    let status = unsafe { HE5_GDgetaliaslist(fileid, HE5_HDFE_DATAGROUP as i32, std::ptr::null_mut(), &mut alias_count) };
    if status < 0 {
        panic!("Error getting alias list");
    }

    let mut alias_list: Vec<String> = vec![String::new(); alias_count as usize];
    let status = unsafe { HE5_GDgetaliaslist(fileid, HE5_HDFE_DATAGROUP as i32, alias_list.as_mut_ptr() as *mut i8, &mut alias_count) };
    if status < 0 {
        panic!("Error getting alias list");
    }

    alias_list
}



#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_open_file() {
        open_hdf5_file("/mnt/data/development/hdf-eos5-rs/sample_data/MOD07_L2.A2024252.0000.061.2024252131104.hdf");
    }

    #[test]
    fn test_get_alias_list() {
        let fileid = open_file("/mnt/data/development/hdf-eos5-rs/sample_data/MOD07_L2.A2024252.0000.061.2024252131104.hdf");
        let alias_list = get_alias_list(fileid);
        println!("{:?}", alias_list);
        panic!();
    }
}
