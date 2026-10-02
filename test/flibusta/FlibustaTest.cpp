#include <OpdsParser.h>
#include <OpdsServerStore.h>
#include <gtest/gtest.h>

#include <fstream>
#include <iterator>

namespace {
std::string savedJson;
bool failSave = false;
int writes = 0;
}  // namespace

bool PersistableStoreBase::writeDocToFile(const char*, const JsonDocument& doc) {
  ++writes;
  if (failSave) return false;
  savedJson.clear();
  serializeJson(doc, savedJson);
  return true;
}
bool PersistableStoreBase::readDocFromFile(const char*, JsonDocument& doc) { return !deserializeJson(doc, savedJson); }
std::string PersistableStoreBase::extractPassword(JsonVariantConst doc, bool&) { return doc["password_obf"] | ""; }

class FlibustaStore : public testing::Test {
 protected:
  void SetUp() override {
    failSave = false;
    writes = 0;
    savedJson = R"({"servers":[]})";
    ASSERT_TRUE(OPDS_STORE.loadFromFile());
  }
};

TEST_F(FlibustaStore, FreshInstallGetsEditableAnonymousHttpsCatalog) {
  ASSERT_TRUE(OPDS_STORE.ensureFlibustaPreset());
  ASSERT_EQ(OPDS_STORE.getCount(), 1u);
  const auto* server = OPDS_STORE.getServer(0);
  EXPECT_EQ(server->name, "Flibusta");
  EXPECT_EQ(server->url, "https://flibusta.is/opds");
  EXPECT_TRUE(server->username.empty());
  EXPECT_TRUE(server->password.empty());
  ASSERT_TRUE(OPDS_STORE.loadFromFile());
  ASSERT_TRUE(OPDS_STORE.ensureFlibustaPreset());
  EXPECT_EQ(OPDS_STORE.getCount(), 1u);
  EXPECT_EQ(writes, 1);
}

TEST_F(FlibustaStore, UpgradePreservesExistingServerAndCredentials) {
  ASSERT_TRUE(OPDS_STORE.addServer({"My books", "https://example.org/opds", "reader", "secret"}));
  ASSERT_TRUE(OPDS_STORE.ensureFlibustaPreset());
  ASSERT_TRUE(OPDS_STORE.loadFromFile());
  ASSERT_EQ(OPDS_STORE.getCount(), 2u);
  const auto* server = OPDS_STORE.getServer(0);
  EXPECT_EQ(server->name, "My books");
  EXPECT_EQ(server->url, "https://example.org/opds");
  EXPECT_EQ(server->username, "reader");
  EXPECT_EQ(server->password, "secret");
}

TEST_F(FlibustaStore, ExistingMirrorIsNotDuplicatedOrRewritten) {
  ASSERT_TRUE(OPDS_STORE.addServer({"My Flibusta", "https://flibusta.site/opds/", "", ""}));
  ASSERT_TRUE(OPDS_STORE.ensureFlibustaPreset());
  ASSERT_EQ(OPDS_STORE.getCount(), 1u);
  EXPECT_EQ(OPDS_STORE.getServer(0)->url, "https://flibusta.site/opds/");
}

TEST_F(FlibustaStore, DeletionSurvivesReboot) {
  ASSERT_TRUE(OPDS_STORE.ensureFlibustaPreset());
  ASSERT_TRUE(OPDS_STORE.removeServer(0));
  ASSERT_TRUE(OPDS_STORE.loadFromFile());
  ASSERT_TRUE(OPDS_STORE.ensureFlibustaPreset());
  EXPECT_EQ(OPDS_STORE.getCount(), 0u);
  EXPECT_EQ(writes, 2);
}

TEST_F(FlibustaStore, FullStoreDoesNotLoseServersAndRetriesWhenSpaceAvailable) {
  for (int i = 0; i < 8; ++i) {
    ASSERT_TRUE(OPDS_STORE.addServer({std::to_string(i), "https://example.org/opds", "", ""}));
  }
  EXPECT_FALSE(OPDS_STORE.ensureFlibustaPreset());
  EXPECT_EQ(OPDS_STORE.getCount(), 8u);
  ASSERT_TRUE(OPDS_STORE.removeServer(7));
  ASSERT_TRUE(OPDS_STORE.loadFromFile());
  ASSERT_TRUE(OPDS_STORE.ensureFlibustaPreset());
  EXPECT_EQ(OPDS_STORE.getCount(), 8u);
  EXPECT_EQ(OPDS_STORE.getServer(7)->name, "Flibusta");
}

TEST_F(FlibustaStore, FailedSaveRollsBackAndCanRetry) {
  failSave = true;
  EXPECT_FALSE(OPDS_STORE.ensureFlibustaPreset());
  EXPECT_EQ(OPDS_STORE.getCount(), 0u);
  failSave = false;
  EXPECT_TRUE(OPDS_STORE.ensureFlibustaPreset());
  EXPECT_EQ(OPDS_STORE.getCount(), 1u);
}

TEST(FlibustaCatalog, SelectsEpubAndSkipsUnsupportedBooksWithRelatedAuthorFeeds) {
  const std::string xml = R"(<feed xmlns="http://www.w3.org/2005/Atom">
    <link rel="search" href="/opds/search?searchTerm={searchTerms}" type="application/atom+xml"/>
    <entry><title>По авторам</title><link href="/opds/authorsindex" type="application/atom+xml"/></entry>
    <entry><title>Евгений Онегин</title><author><name>Пушкин Александр Сергеевич</name></author>
      <link rel="related" href="/opds/author/1" type="application/atom+xml"/>
      <link rel="http://opds-spec.org/acquisition/open-access" href="/b/1/fb2" type="application/fb2+zip"/>
      <link rel="http://opds-spec.org/acquisition/open-access" href="/b/1/epub" type="application/epub+zip"/>
      <link rel="related" href="/opds/author/1" type="application/atom+xml"/>
    </entry>
    <entry><title>PDF only</title>
      <link rel="related" href="/opds/author/2" type="application/atom+xml"/>
      <link rel="http://opds-spec.org/acquisition/open-access" href="/b/2/pdf" type="application/pdf"/>
    </entry>
    <link rel="next" href="/opds/new/0/new?page=1" type="application/atom+xml"/>
  </feed>)";
  OpdsParser parser;
  // Exercise UTF-8 and XML attribute splits across transport chunks.
  for (size_t i = 0; i < xml.size(); i += 7) {
    parser.write(reinterpret_cast<const uint8_t*>(xml.data() + i), std::min(size_t{7}, xml.size() - i));
  }
  parser.flush();
  ASSERT_FALSE(parser.error());
  ASSERT_EQ(parser.getEntries().size(), 2u);
  EXPECT_EQ(parser.getEntries()[0].type, OpdsEntryType::NAVIGATION);
  EXPECT_EQ(parser.getEntries()[1].type, OpdsEntryType::BOOK);
  EXPECT_EQ(parser.getEntries()[1].title, "Евгений Онегин");
  EXPECT_EQ(parser.getEntries()[1].author, "Пушкин Александр Сергеевич");
  EXPECT_EQ(parser.getEntries()[1].href, "/b/1/epub");
  EXPECT_EQ(parser.getSearchTemplate(), "/opds/search?searchTerm={searchTerms}");
  EXPECT_EQ(parser.getNextPageUrl(), "/opds/new/0/new?page=1");
}

TEST(FlibustaCatalog, LiveFeedWhenProvided) {
  const char* path = std::getenv("FLIBUSTA_TEST_FEED");
  if (!path) GTEST_SKIP() << "Set FLIBUSTA_TEST_FEED to a locally fetched catalog";
  std::ifstream file(path, std::ios::binary);
  ASSERT_TRUE(file.good());
  const std::string xml((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  OpdsParser parser;
  parser.write(reinterpret_cast<const uint8_t*>(xml.data()), xml.size());
  parser.flush();
  ASSERT_FALSE(parser.error());
  ASSERT_FALSE(parser.getEntries().empty());
  bool hasBook = false;
  for (const auto& entry : parser.getEntries()) {
    if (entry.type == OpdsEntryType::BOOK) hasBook = true;
  }
  EXPECT_TRUE(hasBook);
}
