//  SuperTux
//  Copyright (C) 2007 Christoph Sommer <christoph.sommer@2007.expires.deltadevelopment.de>
//                2014 Ingo Ruhnke <grumbel@gmail.com>
//                2023 Vankata453
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "downloader.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

#include <physfs.h>

TransferStatus::TransferStatus(Downloader& downloader, TransferId id_,
                               const std::string& url) :
  m_downloader(downloader),
  id(id_),
  file(),
  callbacks(),
  dltotal(0),
  dlnow(0),
  ultotal(0),
  ulnow(0),
  error_msg()
{
  const std::size_t last_idx = url.find_last_of("/");
  const_cast<std::string&>(file) = (last_idx == std::string::npos ? url : url.substr(last_idx + 1));
}

void
TransferStatus::abort()
{
  m_downloader.abort(id);
}

void
TransferStatus::update()
{
  m_downloader.update();
}

class Transfer
{
protected:
  Downloader& m_downloader;
  TransferId m_id;

  std::string m_url;
  CURL* m_handle;
  std::array<char, CURL_ERROR_SIZE> m_error_buffer;

  TransferStatusPtr m_status;

public:
  Transfer(Downloader& downloader, TransferId id,
           const std::string& url) :
    m_downloader(downloader),
    m_id(id),
    m_url(url),
    m_handle(),
    m_error_buffer({{'\0'}}),
    m_status(new TransferStatus(m_downloader, id, url))
  {
    m_handle = curl_easy_init();
    if (!m_handle)
    {
      throw std::runtime_error("curl_easy_init() failed");
    }
    else
    {
      curl_easy_setopt(m_handle, CURLOPT_URL, url.c_str());
      curl_easy_setopt(m_handle, CURLOPT_USERAGENT, "SuperTux/v0.1.3 libcURL");

      curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, this);
      curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, &Transfer::on_data_wrap);

      curl_easy_setopt(m_handle, CURLOPT_ERRORBUFFER, m_error_buffer.data());
      curl_easy_setopt(m_handle, CURLOPT_NOSIGNAL, 1);
      curl_easy_setopt(m_handle, CURLOPT_FAILONERROR, 1);
      curl_easy_setopt(m_handle, CURLOPT_FOLLOWLOCATION, 1);

      curl_easy_setopt(m_handle, CURLOPT_NOPROGRESS, 0);
      curl_easy_setopt(m_handle, CURLOPT_PROGRESSDATA, this);
#if LIBCURL_VERSION_MAJOR > 7 || (LIBCURL_VERSION_MAJOR == 7 && LIBCURL_VERSION_MINOR >= 32)
      curl_easy_setopt(m_handle, CURLOPT_XFERINFOFUNCTION, &Transfer::on_progress_wrap);
#else
      curl_easy_setopt(m_handle, CURLOPT_PROGRESSFUNCTION, &Transfer::on_progress_wrap);
#endif
    }
  }

  virtual ~Transfer()
  {
    curl_easy_cleanup(m_handle);
  }

  TransferStatusPtr get_status() const
  {
    return m_status;
  }

  const char* get_error_buffer() const
  {
    return m_error_buffer.data();
  }

  TransferId get_id() const
  {
    return m_id;
  }

  CURL* get_curl_handle() const
  {
    return m_handle;
  }

  const std::string& get_url() const
  {
    return m_url;
  }

  virtual void on_finished(bool success)
  {
    for (const auto& callback : m_status->callbacks)
    {
      try
      {
        callback(success);
      }
      catch(const std::exception& err)
      {
        printf("DOWNLOADER: Illegal exception in Downloader: %s\n", err.what());
      }
    }
  }

  virtual size_t on_data(const char* ptr, size_t size, size_t nmemb) = 0;

#if LIBCURL_VERSION_MAJOR > 7 || (LIBCURL_VERSION_MAJOR == 7 && LIBCURL_VERSION_MINOR >= 32)
  int on_progress(curl_off_t dltotal, curl_off_t dlnow,
                  curl_off_t ultotal, curl_off_t ulnow)
#else
  int on_progress(double dltotal, double dlnow,
                  double ultotal, double ulnow)
#endif
  {
    m_status->dltotal = static_cast<int>(dltotal);
    m_status->dlnow = static_cast<int>(dlnow);

    m_status->ultotal = static_cast<int>(ultotal);
    m_status->ulnow = static_cast<int>(ulnow);

    return 0;
  }

private:
  static size_t on_data_wrap(const char* ptr, size_t size, size_t nmemb, void* userdata)
  {
    return static_cast<Transfer*>(userdata)->on_data(ptr, size, nmemb);
  }

  static int on_progress_wrap(void* userdata,
#if LIBCURL_VERSION_MAJOR > 7 || (LIBCURL_VERSION_MAJOR == 7 && LIBCURL_VERSION_MINOR >= 32)
                              curl_off_t dltotal, curl_off_t dlnow,
                              curl_off_t ultotal, curl_off_t ulnow)
#else
                              double dltotal, double dlnow,
                              double ultotal, double ulnow)
#endif
  {
    return static_cast<Transfer*>(userdata)->on_progress(dltotal, dlnow, ultotal, ulnow);
  }

private:
  Transfer(const Transfer&) = delete;
  Transfer& operator=(const Transfer&) = delete;
};

class FileTransfer final : public Transfer
{
private:
  std::string m_outfile;
  std::unique_ptr<PHYSFS_file, int(*)(PHYSFS_File*)> m_fout;

public:
  FileTransfer(Downloader& downloader, TransferId id,
               const std::string& url,
               const std::string& outfile) :
    Transfer(downloader, id, url),
    m_outfile(outfile),
    m_fout(PHYSFS_openWrite(outfile.c_str()), PHYSFS_close)
  {
    if (!m_fout)
    {
      char err[256];
      snprintf(err, 256, "TRANSFER: PHYSFS_openRead() failed: %s", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
      throw std::runtime_error(err);
    }
  }

  void on_finished(bool success) override
  {
    Transfer::on_finished(success);

    if (!success)
    {
      m_fout.reset();
      PHYSFS_delete(m_outfile.c_str());
    }
  }

  size_t on_data(const char* ptr, size_t size, size_t nmemb) override
  {
    PHYSFS_writeBytes(m_fout.get(), ptr, size * nmemb);
    return size * nmemb;
  }

private:
  FileTransfer(const FileTransfer&) = delete;
  FileTransfer& operator=(const FileTransfer&) = delete;
};

class StringTransfer final : public Transfer
{
private:
  std::string& m_out;

public:
  StringTransfer(Downloader& downloader, TransferId id,
                 const std::string& url,
                 std::string& out) :
    Transfer(downloader, id, url),
    m_out(out)
  {}

  size_t on_data(const char* ptr, size_t size, size_t nmemb) override
  {
    m_out += std::string(ptr, size * nmemb);
    return size * nmemb;
  }

private:
  StringTransfer(const StringTransfer&) = delete;
  StringTransfer& operator=(const StringTransfer&) = delete;
};

Downloader::Downloader() :
  m_multi_handle(),
  m_transfers(),
  m_next_transfer_id(1)
{
  curl_global_init(CURL_GLOBAL_ALL);
  m_multi_handle = curl_multi_init();
  if (!m_multi_handle)
  {
    throw std::runtime_error("curl_multi_init() failed");
  }
}

Downloader::~Downloader()
{
  for (auto& transfer : m_transfers)
  {
    curl_multi_remove_handle(m_multi_handle, transfer.second->get_curl_handle());
  }
  m_transfers.clear();

  curl_multi_cleanup(m_multi_handle);
  curl_global_cleanup();
}

void
Downloader::abort(TransferId id)
{
  auto it = m_transfers.find(id);
  if (it == m_transfers.end())
  {
    printf("DOWNLOADER: Transfer not found: %d\n", id);
    return;
  }

  it->second->get_status()->error_msg = "Download aborted.";

  printf("DOWNLOADER: Error downloading '%s': Download aborted.\n", it->second->get_url().c_str());
  it->second->on_finished(false);

  curl_multi_remove_handle(m_multi_handle, it->second->get_curl_handle());
  m_transfers.erase(it);
}

void
Downloader::update()
{
  // Read data from the network.
  CURLMcode ret;
  int running_handles;
  while ((ret = curl_multi_perform(m_multi_handle, &running_handles)) == CURLM_CALL_MULTI_PERFORM)
  {
  }

  // Check if any downloads got finished.
  int msgs_in_queue;
  CURLMsg* msg;
  while (msg = curl_multi_info_read(m_multi_handle, &msgs_in_queue))
  {
    switch (msg->msg)
    {
      case CURLMSG_DONE:
        {
          CURLcode resultfromcurl = msg->data.result;
          printf("DOWNLOADER: Download completed with %d\n", resultfromcurl);
          curl_multi_remove_handle(m_multi_handle, msg->easy_handle);

          auto it = std::find_if(m_transfers.begin(), m_transfers.end(),
                                 [&msg](const auto& rhs) {
                                   return rhs.second->get_curl_handle() == msg->easy_handle;
                                 });
          assert(it != m_transfers.end());
          TransferStatusPtr status = it->second->get_status();
          status->error_msg = it->second->get_error_buffer();

          if (resultfromcurl == CURLE_OK)
          {
            it->second->on_finished(true);
          }
          else
          {
            printf("DOWNLOADER: Error downloading '%s': %s\n", it->second->get_url().c_str(), curl_easy_strerror(resultfromcurl));
            it->second->on_finished(false);
          }
          m_transfers.erase(it);
        }
        break;

      default:
        printf("DOWNLOADER: Unhandled cURL message: %s\n", msg->msg);
        break;
    }
  }
}

TransferStatusPtr
Downloader::request_download_file(const std::string& url, const std::string& outfile)
{
  auto transfer = std::make_unique<FileTransfer>(*this, m_next_transfer_id++, url, outfile);

  curl_multi_add_handle(m_multi_handle, transfer->get_curl_handle());

  auto transferId = transfer->get_id();
  m_transfers[transferId] = std::move(transfer);
  return m_transfers[transferId]->get_status();
}

TransferStatusPtr
Downloader::request_download_string(const std::string& url, std::string& out)
{
  auto transfer = std::make_unique<StringTransfer>(*this, m_next_transfer_id++, url, out);

  curl_multi_add_handle(m_multi_handle, transfer->get_curl_handle());

  auto transferId = transfer->get_id();
  m_transfers[transferId] = std::move(transfer);
  return m_transfers[transferId]->get_status();
}
