#ifndef ST_STATS_BUFFER_H
#define ST_STATS_BUFFER_H

typedef struct{
    char *buffer;   /**< start buffer */
    int size;       /**< total size of this buffer */
    int write_pos;  /**< position to write next */
    int read_pos;   /**< position to read next */
    int count;      /**< current amount used in the buffer */
} st_stats_buffer;

/**
 * @brief Allocate buffers for storing instrumentation data
 */
void st_buffer_allocate(void);

/**
 * @brief finish allocation and file set up of given instrumentation mode
 * @param[in] inst_mode instrumentation mode
 */
void buffer_init(int inst_mode);

/**
 * @brief Copy data into the buffer.
 * @param[in] inst_mode instrumentation mode
 * @param[in] data pointer to the data
 * @param[in] size size of the data to be copied
 */
void st_buffer_push(int inst_mode, char *data, int size);

/**
 * @brief Receive a pointer to the buffer for directly writing data
 * @param[in] inst_mode instrumentation mode
 * @param[in] size size of the data to be stored in the buffer
 * @return pointer to the buffer
 */
char* st_buffer_pointer(int inst_mode, size_t size);

/**
 * @brief Write buffer data to file
 * @param[in] end_of_sim flag to denote if this is occurring after the simulation has ended
 * @param[in] inst_mode instrumentation mode
 */
void st_buffer_write(int end_of_sim, int inst_mode);

/**
 * @brief Do any final writing and close file handlers
 * @param[in] inst_mode instrumentation mode
 */
void st_buffer_finalize(int inst_mode);

/**
 * @brief Write some data about number of PEs, KPs, and instrumentation mode at beginning of file
 * @param[in] inst_mode instrumentation mode
 */
void write_file_metadata(int inst_mode);

#endif // ST_STATS_BUFFER_H
