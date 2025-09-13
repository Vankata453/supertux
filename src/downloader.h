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

#pragma once

#include <curl/curl.h>
#include <curl/easy.h>

#include <functional>
#include <memory>
#include <map>
#include <string>
#include <vector>

class Downloader;

class TransferStatus;
using TransferStatusPtr = std::shared_ptr<TransferStatus>;

typedef int TransferId;

class TransferStatus final
{
  friend class Downloader;

public:
  Downloader& m_downloader;
  TransferId id;
  const std::string file;
  std::vector<std::function<void (bool)> > callbacks;

  int dltotal;
  int dlnow;
  int ultotal;
  int ulnow;

  std::string error_msg;

public:
  TransferStatus(Downloader& downloader, TransferId id_,
                 const std::string& url);

  void abort();
  void update();

  void then(const std::function<void (bool)>& callback)
  {
    callbacks.push_back(callback);
  }

private:
  TransferStatus(const TransferStatus&) = delete;
  TransferStatus& operator=(const TransferStatus&) = delete;
};

class Transfer;

class Downloader final
{
private:
  CURLM* m_multi_handle;
  std::map<TransferId, std::unique_ptr<Transfer> > m_transfers;
  int m_next_transfer_id;

public:
  Downloader();
  ~Downloader();

  void update();

  TransferStatusPtr request_download_file(const std::string& url, const std::string& filename);
  TransferStatusPtr request_download_string(const std::string& url, std::string& out);

  void abort(TransferId id);

private:
  Downloader(const Downloader&) = delete;
  Downloader& operator=(const Downloader&) = delete;
};
