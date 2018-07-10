/* Licensed under the Apache License, Version 2.0 (the "License"). You may not use
 * this file except in compliance with the License. A copy of the License is
 * located at
 *
    *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed on an
     * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <aws/cryptosdk/private/utils.h>
#include <assert.h>

int aws_array_list_compare_hash_elements_by_key_string(const void * elem_a, const void * elem_b) {
    const struct aws_hash_element * a = (const struct aws_hash_element *)elem_a;
    const struct aws_hash_element * b = (const struct aws_hash_element *)elem_b;
    const struct aws_string * key_a = (const struct aws_string *)a->key;
    const struct aws_string * key_b = (const struct aws_string *)b->key;
    return aws_string_compare(key_a, key_b);
}

int aws_hash_table_get_elems_array_init(struct aws_allocator * alloc,
                                        struct aws_array_list * elems,
                                        const struct aws_hash_table * map) {
    size_t entry_count = aws_hash_table_get_entry_count(map);
    if (aws_array_list_init_dynamic(elems, alloc, entry_count, sizeof(struct aws_hash_element))) {
        return AWS_OP_ERR;
    }

    for (struct aws_hash_iter iter = aws_hash_iter_begin(map);
         !aws_hash_iter_done(&iter); aws_hash_iter_next(&iter)) {
        if (aws_array_list_push_back(elems, (void **) &iter.element)) {
            aws_array_list_clean_up(elems);
            return AWS_OP_ERR;
        }
    }
    assert(aws_array_list_length(elems) == entry_count);
    return AWS_OP_SUCCESS;
}

bool aws_string_eq_byte_cursor(const struct aws_string * str,
                               const struct aws_byte_cursor * cur) {
    if (str->len != cur->len) return false;
    return (!memcmp(aws_string_bytes(str), cur->ptr, cur->len));
}

bool aws_string_eq_byte_buf(const struct aws_string * str,
                            const struct aws_byte_buf * buf) {
    if (str->len != buf->len) return false;
    return (!memcmp(aws_string_bytes(str), buf->buffer, buf->len));
}
