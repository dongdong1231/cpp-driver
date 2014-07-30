#define BOOST_TEST_DYN_LINK
#ifdef STAND_ALONE
#   define BOOST_TEST_MODULE cassandra
#endif

#define BOOST_THREAD_VERSION 2 
//#define BOOST_NO_CXX11_RVALUE_REFERENCES 1

#include <boost/test/unit_test.hpp>
#include <boost/test/debug.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/cstdint.hpp>
#include <boost/format.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "cassandra.h"
#include "test_utils.hpp"

struct AllTypes {
  test_utils::Uuid id;
  CassString text_sample;
  cass_int32_t int_sample;
  cass_int64_t bigint_sample;
  cass_float_t float_sample;
  cass_double_t double_sample;
  CassDecimal decimal_sample;
  CassBytes blob_sample;
  cass_bool_t boolean_sample;
  cass_int64_t timestamp_sample;
  CassInet inet_sample;
};

struct PreparedTests : public test_utils::SingleSessionTest {
    static const char* ALL_TYPE_TABLE_NAME;

    PreparedTests() : test_utils::SingleSessionTest(2, 0) {
      test_utils::execute_query(session, str(boost::format(test_utils::CREATE_KEYSPACE_SIMPLE_FORMAT)
                                                   % test_utils::SIMPLE_KEYSPACE % "1"));
      test_utils::execute_query(session, str(boost::format("USE %s") % test_utils::SIMPLE_KEYSPACE));
      test_utils::execute_query(session, str(boost::format(test_utils::CREATE_TABLE_ALL_TYPES) % ALL_TYPE_TABLE_NAME));
    }
};

const char* PreparedTests::ALL_TYPE_TABLE_NAME = "all_types_table_prepared";

BOOST_FIXTURE_TEST_SUITE(prepared, PreparedTests)

void insert_all_types(CassSession* session, const CassPrepared* prepared, const AllTypes& all_types) {
  test_utils::CassStatementPtr statement = test_utils::make_shared(cass_prepared_bind(prepared));

  cass_statement_bind_uuid(statement.get(), 0, all_types.id.uuid);
  cass_statement_bind_string(statement.get(), 1, all_types.text_sample);
  cass_statement_bind_int32(statement.get(), 2, all_types.int_sample);
  cass_statement_bind_int64(statement.get(), 3, all_types.bigint_sample);
  cass_statement_bind_float(statement.get(), 4, all_types.float_sample);
  cass_statement_bind_double(statement.get(), 5, all_types.double_sample);
  cass_statement_bind_decimal(statement.get(), 6, all_types.decimal_sample);
  cass_statement_bind_bytes(statement.get(), 7, all_types.blob_sample);
  cass_statement_bind_bool(statement.get(), 8, all_types.boolean_sample);
  cass_statement_bind_int64(statement.get(), 9, all_types.timestamp_sample);
  cass_statement_bind_inet(statement.get(), 10, all_types.inet_sample);

  test_utils::CassFuturePtr future = test_utils::make_shared(cass_session_execute(session, statement.get()));

  test_utils::wait_and_check_error(future.get());
}

void compare_all_types(const AllTypes& input, const CassRow* row) {
  AllTypes output;
  BOOST_REQUIRE(cass_value_get_string(cass_row_get_column(row, 1), &output.text_sample) == CASS_OK);
  BOOST_REQUIRE(test_utils::Value<CassString>::equal(input.text_sample, output.text_sample));

  BOOST_REQUIRE(cass_value_get_int32(cass_row_get_column(row, 2), &output.int_sample) == CASS_OK);
  BOOST_REQUIRE(test_utils::Value<cass_int32_t>::equal(input.int_sample, output.int_sample));

  BOOST_REQUIRE(cass_value_get_int64(cass_row_get_column(row, 3), &output.bigint_sample) == CASS_OK);
  BOOST_REQUIRE(test_utils::Value<cass_int64_t>::equal(input.bigint_sample, output.bigint_sample));

  BOOST_REQUIRE(cass_value_get_float(cass_row_get_column(row, 4), &output.float_sample) == CASS_OK);
  BOOST_REQUIRE(test_utils::Value<cass_float_t>::equal(input.float_sample, output.float_sample));

  BOOST_REQUIRE(cass_value_get_double(cass_row_get_column(row, 5), &output.double_sample) == CASS_OK);
  BOOST_REQUIRE(test_utils::Value<cass_double_t>::equal(input.double_sample, output.double_sample));

  BOOST_REQUIRE(cass_value_get_decimal(cass_row_get_column(row, 6), &output.decimal_sample) == CASS_OK);
  BOOST_REQUIRE(test_utils::Value<CassDecimal>::equal(input.decimal_sample, output.decimal_sample));

  BOOST_REQUIRE(cass_value_get_bytes(cass_row_get_column(row, 7), &output.blob_sample) == CASS_OK);
  BOOST_REQUIRE(test_utils::Value<CassBytes>::equal(input.blob_sample, output.blob_sample));

  BOOST_REQUIRE(cass_value_get_bool(cass_row_get_column(row, 8), &output.boolean_sample) == CASS_OK);
  BOOST_REQUIRE(test_utils::Value<cass_bool_t>::equal(input.boolean_sample, output.boolean_sample));

  BOOST_REQUIRE(cass_value_get_int64(cass_row_get_column(row, 9), &output.timestamp_sample) == CASS_OK);
  BOOST_REQUIRE(test_utils::Value<cass_int64_t>::equal(input.timestamp_sample, output.timestamp_sample));

  BOOST_REQUIRE(cass_value_get_inet(cass_row_get_column(row, 10), &output.inet_sample) == CASS_OK);
  BOOST_REQUIRE(test_utils::Value<CassInet>::equal(input.inet_sample, output.inet_sample));
}

BOOST_AUTO_TEST_CASE(test_bound_all_types_different_values)
{
  std::string insert_query = str(boost::format("INSERT INTO %s "
                                               "(id, text_sample, int_sample, bigint_sample, float_sample, double_sample, decimal_sample, "
                                               "blob_sample, boolean_sample, timestamp_sample, inet_sample) "
                                               "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)") % ALL_TYPE_TABLE_NAME);

  test_utils::CassFuturePtr prepared_future = test_utils::make_shared(cass_session_prepare(session,
                                                                                           cass_string_init2(insert_query.data(), insert_query.size())));
  test_utils::wait_and_check_error(prepared_future.get());
  test_utils::CassPreparedPtr prepared = test_utils::make_shared(cass_future_get_prepared(prepared_future.get()));

  uint8_t varint1[] = { 1, 2, 3 };
  uint8_t varint2[] = { 0, 0, 0 };
  uint8_t varint3[] = { 255, 255, 255, 255, 255 };
  uint8_t bytes1[] = { 255, 255 };
  uint8_t bytes2[] = { 0, 0 };
  uint8_t bytes3[] = { 1, 1 };
  uint8_t address1[CASS_INET_V4_LENGTH] = { 192, 168, 0, 100 };
  uint8_t address2[CASS_INET_V4_LENGTH] = { 0, 0, 0, 0 };
  uint8_t address3[CASS_INET_V6_LENGTH] = { 255, 128, 12, 1, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };

  const size_t all_types_count = 3;
  AllTypes all_types[all_types_count];

  all_types[0].id = test_utils::generate_time_uuid();
  all_types[0].text_sample = cass_string_init("first");
  all_types[0].int_sample = 10;
  all_types[0].bigint_sample = INT64_MAX - 1L;
  all_types[0].float_sample = 1.999f;
  all_types[0].double_sample = 32.002;
  all_types[0].decimal_sample = cass_decimal_init(1, cass_bytes_init(varint1, sizeof(varint1)));
  all_types[0].blob_sample = cass_bytes_init(bytes1, sizeof(bytes1));
  all_types[0].boolean_sample = cass_true;
  all_types[0].timestamp_sample = 1123200000;
  all_types[0].inet_sample = cass_inet_init_v4(address1);

  all_types[1].id = test_utils::generate_time_uuid();
  all_types[1].text_sample = cass_string_init("second");
  all_types[1].int_sample = 0;
  all_types[1].bigint_sample = 0L;
  all_types[1].float_sample = 0.0f;
  all_types[1].double_sample = 0.0;
  all_types[1].decimal_sample = cass_decimal_init(2, cass_bytes_init(varint2, sizeof(varint2)));
  all_types[1].blob_sample = cass_bytes_init(bytes2, sizeof(bytes2));
  all_types[1].boolean_sample = cass_false;
  all_types[1].timestamp_sample = 0;
  all_types[1].inet_sample = cass_inet_init_v4(address2);

  all_types[2].id = test_utils::generate_time_uuid();
  all_types[2].text_sample = cass_string_init("third");
  all_types[2].int_sample = -100;
  all_types[2].bigint_sample = INT64_MIN + 1;
  all_types[2].float_sample = -150.111f;
  all_types[2].double_sample = -5.12342;
  all_types[2].decimal_sample = cass_decimal_init(3, cass_bytes_init(varint3, sizeof(varint3)));
  all_types[2].blob_sample = cass_bytes_init(bytes3, sizeof(bytes3));
  all_types[2].boolean_sample = cass_true;
  all_types[2].timestamp_sample = -13462502400;
  all_types[2].inet_sample = cass_inet_init_v4(address3);

  for(size_t i = 0; i < all_types_count; ++i) {
    insert_all_types(session, prepared.get(), all_types[i]);
  }

  std::string select_query = str(boost::format("SELECT "
                                               "id, text_sample, int_sample, bigint_sample, float_sample, double_sample, decimal_sample, "
                                               "blob_sample, boolean_sample, timestamp_sample, inet_sample "
                                               "FROM %s WHERE id IN (%s, %s, %s)")
                                 % ALL_TYPE_TABLE_NAME
                                 % test_utils::string_from_uuid(all_types[0].id)
                                 % test_utils::string_from_uuid(all_types[1].id)
                                 % test_utils::string_from_uuid(all_types[2].id));

  test_utils::CassResultPtr result;
  test_utils::execute_query(session, select_query, &result);
  BOOST_REQUIRE(cass_result_row_count(result.get()) == all_types_count);
  BOOST_REQUIRE(cass_result_column_count(result.get()) == 11);

  test_utils::CassIteratorPtr iterator = test_utils::make_shared(cass_iterator_from_result(result.get()));

  while(cass_iterator_next(iterator.get())) {
    const CassRow* row = cass_iterator_get_row(iterator.get());
    CassUuid id;
    cass_value_get_uuid(cass_row_get_column(row, 0), id);
    for(size_t i = 0; i < all_types_count; ++i) {
      if(test_utils::Value<CassUuid>::equal(id, all_types[i].id)) {
        compare_all_types(all_types[i], row);
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(test_bound_all_types_null_values)
{
  std::string insert_query = str(boost::format("INSERT INTO %s "
                                               "(id, text_sample, int_sample, bigint_sample, float_sample, double_sample, decimal_sample, "
                                               "blob_sample, boolean_sample, timestamp_sample, inet_sample) "
                                               "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)") % ALL_TYPE_TABLE_NAME);

  test_utils::CassFuturePtr prepared_future = test_utils::make_shared(cass_session_prepare(session,
                                                                        cass_string_init2(insert_query.data(), insert_query.size())));
  test_utils::wait_and_check_error(prepared_future.get());
  test_utils::CassPreparedPtr prepared = test_utils::make_shared(cass_future_get_prepared(prepared_future.get()));

  test_utils::CassStatementPtr statement = test_utils::make_shared(cass_prepared_bind(prepared.get()));

  test_utils::Uuid id = test_utils::generate_time_uuid();

  cass_statement_bind_uuid(statement.get(), 0, id);
  cass_statement_bind_null(statement.get(), 1);
  cass_statement_bind_null(statement.get(), 2);
  cass_statement_bind_null(statement.get(), 3);
  cass_statement_bind_null(statement.get(), 4);
  cass_statement_bind_null(statement.get(), 5);
  cass_statement_bind_null(statement.get(), 6);
  cass_statement_bind_null(statement.get(), 7);
  cass_statement_bind_null(statement.get(), 8);
  cass_statement_bind_null(statement.get(), 9);
  cass_statement_bind_null(statement.get(), 10);

  test_utils::CassFuturePtr future = test_utils::make_shared(cass_session_execute(session, statement.get()));

  test_utils::wait_and_check_error(future.get());

  std::string select_query = str(boost::format("SELECT "
                                               "id, text_sample, int_sample, bigint_sample, float_sample, double_sample, decimal_sample, "
                                               "blob_sample, boolean_sample, timestamp_sample, inet_sample "
                                               "FROM %s WHERE id IN (%s)")
                                 % ALL_TYPE_TABLE_NAME
                                 % test_utils::string_from_uuid(id));

  test_utils::CassResultPtr result;
  test_utils::execute_query(session, select_query, &result);
  BOOST_REQUIRE(cass_result_row_count(result.get()) == 1);
  BOOST_REQUIRE(cass_result_column_count(result.get()) == 11);

  const CassRow* row = cass_result_first_row(result.get());
  test_utils::Uuid result_id;
  BOOST_REQUIRE(cass_value_get_uuid(cass_row_get_column(row, 0), result_id) == CASS_OK);
  BOOST_REQUIRE(test_utils::Value<CassUuid>::equal(id, result_id));
  for(size_t i = 1; i < 11; ++i) {
    BOOST_REQUIRE(cass_value_is_null(cass_row_get_column(row, i)));
  }
}

BOOST_AUTO_TEST_CASE(test_select_one)
{
  std::string table_name = str(boost::format("table_%s") % test_utils::generate_unique_str());
  std::string create_table_query = str(boost::format("CREATE TABLE %s (tweet_id int PRIMARY KEY, numb double, label text);") % table_name);

  test_utils::execute_query(session, create_table_query);

  for(int i = 0; i < 10; ++i) {
    std::string insert_query = str(boost::format("INSERT INTO %s (tweet_id, numb, label) VALUES(%d, 0.01,'row%d')") % table_name % i % i);
    test_utils::execute_query(session, insert_query);
  }

  std::string select_query = str(boost::format("SELECT * FROM %s WHERE tweet_id = ?;") % table_name);

  test_utils::CassFuturePtr prepared_future = test_utils::make_shared(cass_session_prepare(session,
                                                                 cass_string_init2(select_query.data(), select_query.size())));
  test_utils::wait_and_check_error(prepared_future.get());
  test_utils::CassPreparedPtr prepared = test_utils::make_shared(cass_future_get_prepared(prepared_future.get()));

  int tweet_id = 5;
  test_utils::CassStatementPtr statement = test_utils::make_shared(cass_prepared_bind(prepared.get()));
  BOOST_REQUIRE(cass_statement_bind_int32(statement.get(), 0, tweet_id) == CASS_OK);

  test_utils::CassFuturePtr future = test_utils::make_shared(cass_session_execute(session, statement.get()));
  test_utils::wait_and_check_error(future.get());

  test_utils::CassResultPtr result = test_utils::make_shared(cass_future_get_result(future.get()));
  BOOST_REQUIRE(cass_result_row_count(result.get()) == 1);
  BOOST_REQUIRE(cass_result_column_count(result.get()) == 3);

  const CassRow* row = cass_result_first_row(result.get());
  int result_tweet_id;
  CassString result_label;
  BOOST_REQUIRE(cass_value_get_int32(cass_row_get_column(row, 0), &result_tweet_id) == CASS_OK);
  BOOST_REQUIRE(test_utils::Value<cass_int32_t>::equal(tweet_id, result_tweet_id));
  BOOST_REQUIRE(cass_value_get_string(cass_row_get_column(row, 1), &result_label) == CASS_OK);
  BOOST_REQUIRE(test_utils::Value<CassString>::equal(cass_string_init("row5"), result_label));
}

const CassPrepared* prepare_statement(CassSession* session, std::string query) {
  test_utils::CassFuturePtr prepared_future = test_utils::make_shared(cass_session_prepare(session,
                                                                        cass_string_init2(query.data(), query.size())));
  test_utils::wait_and_check_error(prepared_future.get());
  return cass_future_get_prepared(prepared_future.get());
}

void execute_statement(CassSession* session, const CassPrepared* prepared, int value) {
  test_utils::CassStatementPtr statement = test_utils::make_shared(cass_prepared_bind(prepared));
  BOOST_REQUIRE(cass_statement_bind_double(statement.get(), 0, static_cast<double>(value)) == CASS_OK);
  BOOST_REQUIRE(cass_statement_bind_int32(statement.get(), 1, value) == CASS_OK);
  test_utils::CassFuturePtr future = test_utils::make_shared(cass_session_execute(session, statement.get()));
  test_utils::wait_and_check_error(future.get());
}

BOOST_AUTO_TEST_CASE(test_massive_number_of_prepares)
{
  std::string table_name = str(boost::format("table_%s") % test_utils::generate_unique_str());
  std::string create_table_query = str(boost::format("CREATE TABLE %s (tweet_id uuid PRIMARY KEY, numb1 double, numb2 int);") % table_name);
  size_t number_of_prepares = 100;

  test_utils::execute_query(session, create_table_query);

  std::vector<boost::shared_future<const CassPrepared*> > prepare_futures;
  std::vector<test_utils::Uuid> tweet_ids;
  for(size_t i = 0; i < number_of_prepares; ++i) {
    test_utils::Uuid tweet_id = test_utils::generate_time_uuid();
    std::string insert_query = str(boost::format("INSERT INTO %s (tweet_id, numb1, numb2) VALUES (%s, ?, ?);") % table_name % test_utils::string_from_uuid(tweet_id));
    boost::shared_future<const CassPrepared*> future(boost::async(boost::launch::async, boost::bind(prepare_statement, session, insert_query)));
    prepare_futures.push_back(future);
    tweet_ids.push_back(tweet_id);
  }

  std::vector<boost::shared_future<void> > execute_futures;
  std::vector<test_utils::CassPreparedPtr> prepares;
  for(size_t i = 0; i < prepare_futures.size(); ++i) {
    test_utils::CassPreparedPtr prepared = test_utils::make_shared(prepare_futures[i].get());
    boost::shared_future<void> future(boost::async(boost::launch::async, boost::bind(execute_statement, session, prepared.get(), i)));
    execute_futures.push_back(future);
    prepares.push_back(prepared);
  }

  boost::wait_for_all(execute_futures.begin(), execute_futures.end());

  std::string select_query = str(boost::format("SELECT * FROM %s;") % table_name);
  test_utils::CassResultPtr result;
  test_utils::execute_query(session, select_query, &result);
  BOOST_REQUIRE(cass_result_row_count(result.get()) == number_of_prepares);

  test_utils::CassIteratorPtr iterator = test_utils::make_shared(cass_iterator_from_result(result.get()));

  while(cass_iterator_next(iterator.get())) {
    const CassRow* row = cass_iterator_get_row(iterator.get());
    test_utils::Uuid result_tweet_id;
    cass_value_get_uuid(cass_row_get_column(row, 0), result_tweet_id.uuid);
    BOOST_REQUIRE(std::find(tweet_ids.begin(), tweet_ids.end(), result_tweet_id) != tweet_ids.end());
  }
}

BOOST_AUTO_TEST_SUITE_END()